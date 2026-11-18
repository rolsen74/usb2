
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define DOT4_TIMEOUT 2000000

static U32
Dot4_Length ( const U8 * packet )
{
	return ( ( (U32)packet[2] << 8 ) | packet[3] );
}

static void
Dot4_Header ( U8 * packet, U8 socket, U32 length, U8 credit )
{
	packet[0] = socket;
	packet[1] = socket;
	packet[2] = length >> 8;
	packet[3] = length;
	packet[4] = credit;
	packet[5] = 0;
}

static S32
Dot4_Send ( struct PrinterUnit * unit, U8 * packet, U32 length )
{
	struct USB2_IORequest * io;

	io = unit->unit_Bulk_Tx_Resource->IORequests[0];

	io->io_Command = CMD_WRITE;
	io->io_Data	   = packet;
	io->io_Length  = length;
	io->io_Actual  = 0;
	io->io_TimeOut = DOT4_TIMEOUT;

	DoIO ( (PTR)io );

	if ( ( io->io_Error == USB2Err_Device_Detached ) || ( io->io_Error == USB2Err_Host_Timeout ) )
	{
		unit->unit_Detached = TRUE;
	}

	return ( ( io->io_Error == USB2Err_NoError ) && ( io->io_Actual == length ) );
}

static S32
Dot4_Receive ( struct PrinterUnit * unit, U8 * packet, U32 capacity )
{
	struct USB2_IORequest * io;

	U8	response[10];
	U32 length;

	io = unit->unit_Bulk_Rx_Resource->IORequests[0];

	for ( ;; )
	{
		if ( unit->unit_Dot4_RxUsed >= DOT4_HEADER_SIZE )
		{
			length = Dot4_Length ( unit->unit_Dot4_RxBuffer );

			if ( ( length < DOT4_HEADER_SIZE ) || ( length > DOT4_MAX_PACKET ) )
			{
				return ( FALSE );
			}

			if ( unit->unit_Dot4_RxUsed >= length )
			{
				if ( length > capacity )
				{
					return ( FALSE );
				}

				memcpy ( packet, unit->unit_Dot4_RxBuffer, length );

				unit->unit_Dot4_RxUsed -= length;

				memmove ( unit->unit_Dot4_RxBuffer, unit->unit_Dot4_RxBuffer + length, unit->unit_Dot4_RxUsed );

				break;
			}
		}

		if ( unit->unit_Dot4_RxUsed == DOT4_MAX_PACKET )
		{
			return ( FALSE );
		}

		io->io_Command			 = CMD_READ;
		io->io_Data				 = unit->unit_Dot4_RxBuffer + unit->unit_Dot4_RxUsed;
		io->io_Length			 = DOT4_MAX_PACKET - unit->unit_Dot4_RxUsed;
		io->io_Actual			 = 0;
		io->io_TimeOut			 = DOT4_TIMEOUT;
		io->io_AllowShortPackets = TRUE;

		DoIO ( (PTR)io );

		if ( io->io_Error == USB2Err_Device_Detached )
		{
			unit->unit_Detached = TRUE;
		}

		if ( ( io->io_Error != USB2Err_NoError ) || ( ! io->io_Actual ) )
		{
			return ( FALSE );
		}

		unit->unit_Dot4_RxUsed += io->io_Actual;
	}

	if ( unit->unit_Dot4_Ready && ( packet[0] == unit->unit_Dot4_Socket ) && ( packet[1] == unit->unit_Dot4_Socket ) )
	{
		unit->unit_Dot4_Credits += packet[4];
	}

	if ( ( packet[0] == 0 ) && ( packet[1] == 0 ) && ( Dot4_Length ( packet ) >= 11 ) && ( packet[6] == DOT4_CMD_CREDIT_CHANNEL )
		 && ( packet[7] == unit->unit_Dot4_Socket ) && ( packet[8] == unit->unit_Dot4_Socket ) )
	{
		unit->unit_Dot4_Credits += ( (U32)packet[9] << 8 ) | packet[10];

		Dot4_Header ( response, 0, sizeof ( response ), 1 );

		response[6] = DOT4_CMD_CREDIT_CHANNEL | 0x80;
		response[7] = 0;
		response[8] = unit->unit_Dot4_Socket;
		response[9] = unit->unit_Dot4_Socket;

		if ( ! Dot4_Send ( unit, response, sizeof ( response ) ) )
		{
			return ( FALSE );
		}
	}

	return ( TRUE );
}

static S32
Dot4_Exchange ( struct PrinterUnit * unit, U8 * request, U32 length, U8 command, U8 * reply, U32 capacity )
{
	U32 attempt;

	if ( ! Dot4_Send ( unit, request, length ) )
	{
		return ( FALSE );
	}

	for ( attempt = 0; attempt < 4; attempt++ )
	{
		if ( ! Dot4_Receive ( unit, reply, capacity ) )
		{
			return ( FALSE );
		}

		if ( ( reply[0] == 0 ) && ( reply[1] == 0 ) && ( Dot4_Length ( reply ) >= 8 ) && ( reply[6] == ( command | 0x80 ) ) )
		{
			return ( reply[7] == 0 );
		}
	}

	return ( FALSE );
}

S32
Dot4_Setup ( struct PrinterUnit * unit )
{
	U8	request[16];
	U8	reply[64];
	U32 packet_size;

	Dot4_Header ( request, 0, 8, 1 );

	request[6] = DOT4_CMD_INIT;
	request[7] = 0x20;

	if ( ( ! Dot4_Exchange ( unit, request, 8, DOT4_CMD_INIT, reply, sizeof ( reply ) ) ) || ( Dot4_Length ( reply ) < 9 )
		 || ( reply[8] != 0x20 ) )
	{
		USBERROR ( "Printer : DOT4 init failed" );

		return ( FALSE );
	}

	Dot4_Header ( request, 0, 12, 1 );

	request[6] = DOT4_CMD_GET_SOCKET_ID;

	memcpy ( &request[7], "PRINT", 5 );

	if ( ( ! Dot4_Exchange ( unit, request, 12, DOT4_CMD_GET_SOCKET_ID, reply, sizeof ( reply ) ) )
		 || ( Dot4_Length ( reply ) < 9 ) )
	{
		USBERROR ( "Printer : DOT4 PRINT socket lookup failed" );

		return ( FALSE );
	}

	unit->unit_Dot4_Socket = reply[8];

	Dot4_Header ( request, 0, 15, 1 );

	request[6]	= DOT4_CMD_OPEN_CHANNEL;
	request[7]	= unit->unit_Dot4_Socket;
	request[8]	= unit->unit_Dot4_Socket;
	request[9]	= 0x10;
	request[10] = 0x00;
	request[11] = 0x10;
	request[12] = 0x00;
	request[13] = 0x00;
	request[14] = 0x10;

	if ( ( ! Dot4_Exchange ( unit, request, 15, DOT4_CMD_OPEN_CHANNEL, reply, sizeof ( reply ) ) )
		 || ( Dot4_Length ( reply ) < 18 ) )
	{
		USBERROR ( "Printer : DOT4 PRINT channel open failed" );

		return ( FALSE );
	}

	packet_size = ( (U32)reply[10] << 8 ) | reply[11];

	if ( packet_size <= DOT4_HEADER_SIZE )
	{
		return ( FALSE );
	}

	unit->unit_Dot4_PacketSize = ( packet_size > DOT4_MAX_PACKET ) ? DOT4_MAX_PACKET : packet_size;
	unit->unit_Dot4_Credits += ( (U32)reply[16] << 8 ) | reply[17];
	unit->unit_Dot4_Ready = TRUE;

	USBINFO ( "Printer : DOT4 PRINT channel ready, packet size %lu, credits %lu", unit->unit_Dot4_PacketSize,
			  unit->unit_Dot4_Credits );

	return ( TRUE );
}

S32
Dot4_Write ( struct PrinterUnit * unit, const U8 * data, U32 size, U32 * written )
{
	U8	packet[DOT4_MAX_PACKET];
	U32 length;
	U32 attempts;

	*written = 0;

	while ( *written < size )
	{
		attempts = 0;
		while ( ! unit->unit_Dot4_Credits )
		{
			if ( ( attempts++ >= 4 ) || ( ! Dot4_Receive ( unit, packet, sizeof ( packet ) ) ) )
			{
				return ( FALSE );
			}
		}

		length = size - *written;

		if ( length > unit->unit_Dot4_PacketSize - DOT4_HEADER_SIZE )
		{
			length = unit->unit_Dot4_PacketSize - DOT4_HEADER_SIZE;
		}

		Dot4_Header ( packet, unit->unit_Dot4_Socket, length + DOT4_HEADER_SIZE, 0 );

		memcpy ( &packet[DOT4_HEADER_SIZE], data + *written, length );

		if ( ! Dot4_Send ( unit, packet, length + DOT4_HEADER_SIZE ) )
		{
			return ( FALSE );
		}

		unit->unit_Dot4_Credits--;
		*written += length;
	}

	return ( TRUE );
}

void
Dot4_Shutdown ( struct PrinterUnit * unit )
{
	U8 request[9];
	U8 reply[64];

	Dot4_Header ( request, 0, 9, 1 );

	request[6] = DOT4_CMD_CLOSE_CHANNEL;
	request[7] = unit->unit_Dot4_Socket;
	request[8] = unit->unit_Dot4_Socket;

	Dot4_Exchange ( unit, request, 9, DOT4_CMD_CLOSE_CHANNEL, reply, sizeof ( reply ) );

	Dot4_Header ( request, 0, 7, 1 );

	request[6] = DOT4_CMD_EXIT;

	Dot4_Exchange ( unit, request, 7, DOT4_CMD_EXIT, reply, sizeof ( reply ) );

	unit->unit_Dot4_Ready = FALSE;
}
