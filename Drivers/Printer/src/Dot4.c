
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define DOT4_IO_US 2000000
#define DOT4_EXCHANGE_US 4000000
#define DOT4_PASSIVE_US 250000
#define DOT4_CREDIT_RETRY_US 250000
#define DOT4_CREDIT_TRIES 4
#define DOT4_MAX_CREDITS 65535

// --

static U32
Dot4_Now( struct PrinterUnit *unit )
{
	struct TimeVal now;
	struct TimerIFace *clock;

	clock = unit->unit_Dot4_Clock;

	clock->GetUpTime( &now );

	return ( now.Seconds * 1000000U + now.Microseconds );
}

// --

static U32
Dot4_Word( const U8 *data )
{
	return ( ( (U32)data[0] << 8 ) | data[1] );
}

// --

static void
Dot4_Header( U8 *packet, U8 socket, U32 length, U8 credit )
{
	packet[0] = socket;
	packet[1] = socket;
	packet[2] = length >> 8;
	packet[3] = length;
	packet[4] = credit;
	packet[5] = 0;
}

// --

static void
Dot4_Credits( struct PrinterUnit *unit, U32 credits )
{
	unit->unit_Dot4_Credits = MIN( DOT4_MAX_CREDITS, unit->unit_Dot4_Credits + credits );
}

// --

static void
Dot4_Finish( struct PrinterUnit *unit, S32 error )
{
	struct IOStdReq *request;

	request = unit->unit_Dot4_Request;
	unit->unit_Dot4_Request = NULL;
	unit->unit_Dot4_Stage = DOT4_Idle;
	unit->unit_Dot4_Aborting = FALSE;

	if ( request )
	{
		request->io_Error = error;

		Printer_Reply( (PTR)request );
	}
}

// --

static void
Dot4_Fail( struct PrinterUnit *unit )
{
	unit->unit_Dot4_Dirty = TRUE;

	if ( ( unit->unit_Dot4_Request ) && ( ! unit->unit_Detached ) )
	{
		unit->unit_JobError = ParErr_LineErr;
	}

	Dot4_Finish( unit, unit->unit_Detached ? ParErr_NotOpen : ParErr_LineErr );
}

// --

static void
Dot4_Begin( struct PrinterUnit *unit, enum Dot4Stage stage )
{
	U32 delay;

	unit->unit_Dot4_Stage = stage;
	delay = ( stage == DOT4_Passive ) ? DOT4_PASSIVE_US : DOT4_EXCHANGE_US;

	unit->unit_Dot4_Deadline = Dot4_Now( unit ) + delay;

	switch ( stage )
	{
		case DOT4_Reset:
		case DOT4_ClearOut:
		case DOT4_ClearIn:
			unit->unit_Dot4_Action = DOT4_Control;
			break;

		case DOT4_Passive:
			unit->unit_Dot4_Action = DOT4_Receive;
			break;

		case DOT4_SendData:
			unit->unit_Dot4_Action = DOT4_Payload;
			break;

		default:
			unit->unit_Dot4_Action = DOT4_Command;
			break;
	}
}

// --

static void
Dot4_Submit( struct PrinterUnit *unit, struct USB2_IORequest *io, U32 command, PTR data, U32 length, U32 timeout )
{
	io->io_Command = command;
	io->io_Data = data;
	io->io_Length = length;
	io->io_Actual = 0;
	io->io_TimeOut = timeout;
	io->io_AllowShortPackets = command == CMD_READ;
	unit->unit_Dot4_IO = io;

	SendIO( (PTR)io );
}

// --

static void
Dot4_ControlIO( struct PrinterUnit *unit, U32 timeout )
{
	struct USB2_EPResource *resource;
	struct USB2_SetupData *setup;

	resource = unit->unit_USB_Register->Res_Control;
	setup = resource->SetupData;
	setup->Length = LE_SWAP16( 0 );

	if ( unit->unit_Dot4_Stage == DOT4_Reset )
	{
		setup->RequestType = REQTYPE_Write | REQTYPE_Class | REQTYPE_Interface;
		setup->RequestCode = USBPRT_REQC_SOFT_RESET;
		setup->Value = LE_SWAP16( 0 );
		setup->Index = LE_SWAP16( unit->unit_USB_Interface->IFC_Num );
	}
	else
	{
		setup->RequestType = REQTYPE_Write | REQTYPE_Standard | REQTYPE_EndPoint;
		setup->RequestCode = REQCODE_Clear_Feature;
		setup->Value = LE_SWAP16( USBFEATURE_EndPoint_Halt );
		setup->Index = LE_SWAP16( unit->unit_Dot4_Stage == DOT4_ClearOut ?
			unit->unit_Dot4_OutAddress : unit->unit_Dot4_InAddress );
	}

	Dot4_Submit( unit, resource->IORequests[0], CMD_WRITE, NULL, 0, timeout );
}

// --

static U8
Dot4_Opcode( enum Dot4Stage stage )
{
	switch ( stage )
	{
		case DOT4_Init: return ( DOT4_CMD_INIT );
		case DOT4_Socket: return ( DOT4_CMD_GET_SOCKET_ID );
		case DOT4_Open: return ( DOT4_CMD_OPEN_CHANNEL );
		case DOT4_Credit: return ( DOT4_CMD_CREDIT_REQUEST );
		case DOT4_Close: return ( DOT4_CMD_CLOSE_CHANNEL );
		case DOT4_Exit: return ( DOT4_CMD_EXIT );
		default: return ( 0xff );
	}
}

// --

static void
Dot4_CommandIO( struct PrinterUnit *unit, U32 timeout )
{
	U8 *packet;
	U32 length;

	packet = unit->unit_Dot4_TxBuffer;
	length = 7;
	packet[6] = Dot4_Opcode( unit->unit_Dot4_Stage );

	switch ( unit->unit_Dot4_Stage )
	{
		case DOT4_Init:
			length = 8;
			packet[7] = 0x20;
			break;

		case DOT4_Socket:
			length = 12;

			memcpy( &packet[7], "PRINT", 5 );
			break;

		case DOT4_Open:
			length = 15;
			packet[7] = unit->unit_Dot4_Socket;
			packet[8] = unit->unit_Dot4_Socket;
			packet[9] = DOT4_MAX_PACKET >> 8;
			packet[10] = DOT4_MAX_PACKET & 0xff;
			packet[11] = packet[9];
			packet[12] = packet[10];
			packet[13] = 0;
			packet[14] = 16;
			break;

		case DOT4_Credit:
			length = 11;
			packet[7] = unit->unit_Dot4_Socket;
			packet[8] = unit->unit_Dot4_Socket;
			packet[9] = 0;
			packet[10] = 16;
			break;

		case DOT4_Close:
			length = 9;
			packet[7] = unit->unit_Dot4_Socket;
			packet[8] = unit->unit_Dot4_Socket;
			break;

		default:
			break;
	}

	Dot4_Header( packet, 0, length, 1 );

	Dot4_Submit( unit, unit->unit_Bulk_Tx_Resource->IORequests[0], CMD_WRITE, packet, length, timeout );
}

// --

static void
Dot4_ClearSession( struct PrinterUnit *unit )
{
	unit->unit_Dot4_Ready = FALSE;
	unit->unit_Dot4_Initialized = FALSE;
	unit->unit_Dot4_ChannelOpen = FALSE;
	unit->unit_Dot4_RequestCredit = FALSE;
	unit->unit_Dot4_Credits = 0;
	unit->unit_Dot4_RxUsed = 0;
	unit->unit_Dot4_Socket = 0;
}

// --

static void
Dot4_Reply( struct PrinterUnit *unit, U8 *packet, U32 length )
{
	U32 maximum;

	switch ( unit->unit_Dot4_Stage )
	{
		case DOT4_Init:
			unit->unit_Dot4_Initialized = TRUE;

			if ( ( length >= 9 ) && ( packet[8] == 0x20 ) )
			{
				Dot4_Begin( unit, DOT4_Socket );

				return;
			}
			break;

		case DOT4_Socket:
			if ( ( length >= 9 ) && ( packet[8] ) )
			{
				unit->unit_Dot4_Socket = packet[8];

				Dot4_Begin( unit, DOT4_Open );

				return;
			}
			break;

		case DOT4_Open:
			unit->unit_Dot4_ChannelOpen = TRUE;

			if ( ( length >= 18 ) && ( packet[8] == unit->unit_Dot4_Socket ) &&
				( packet[9] == unit->unit_Dot4_Socket ) )
			{
				maximum = Dot4_Word( &packet[10] );

				if ( maximum > DOT4_HEADER_SIZE )
				{
					unit->unit_Dot4_PacketSize = MIN( maximum, DOT4_MAX_PACKET );
					unit->unit_Dot4_Credits = Dot4_Word( &packet[16] );
					unit->unit_Dot4_Ready = TRUE;
					unit->unit_Dot4_Dirty = FALSE;
					unit->unit_Dot4_Stage = DOT4_Idle;

					return;
				}
			}
			break;

		case DOT4_Credit:
			if ( ( length >= 12 ) && ( packet[8] == unit->unit_Dot4_Socket ) &&
				( packet[9] == unit->unit_Dot4_Socket ) )
			{
				Dot4_Credits( unit, Dot4_Word( &packet[10] ) );

				unit->unit_Dot4_Stage = DOT4_Idle;

				if ( ! unit->unit_Dot4_Credits )
				{
					Printer_TimerArm( &unit->unit_IdleTimer, DOT4_CREDIT_RETRY_US );
				}

				return;
			}
			break;

		case DOT4_Close:
			unit->unit_Dot4_ChannelOpen = FALSE;
			unit->unit_Dot4_Stage = DOT4_Idle;

			return;

		case DOT4_Exit:
			Dot4_ClearSession( unit );

			unit->unit_Dot4_Stage = DOT4_Idle;

			return;

		default:
			break;
	}

	Dot4_Fail( unit );
}

// --

static void
Dot4_Packet( struct PrinterUnit *unit, U8 *packet, U32 length )
{
	U8 command;
	U8 socket;

	socket = unit->unit_Dot4_Socket;

	if ( ( ! packet[0] ) && ( ! packet[1] ) && ( length >= 8 ) )
	{
		command = packet[6];

		if ( ( unit->unit_Dot4_ChannelOpen ) && ( command == DOT4_CMD_CREDIT_CHANNEL ) &&
			( length >= 11 ) && ( packet[7] == socket ) && ( packet[8] == socket ) )
		{
			Dot4_Credits( unit, Dot4_Word( &packet[9] ) );

			unit->unit_Dot4_Action = DOT4_Acknowledge;
		}
		else if ( ( unit->unit_Dot4_Stage != DOT4_Passive ) &&
			( command == ( Dot4_Opcode( unit->unit_Dot4_Stage ) | 0x80 ) ) )
		{
			if ( packet[7] )
			{
				Dot4_Fail( unit );
			}
			else
			{
				Dot4_Reply( unit, packet, length );
			}
		}
	}
	else if ( ( unit->unit_Dot4_ChannelOpen ) && ( packet[0] == socket ) && ( packet[1] == socket ) )
	{
		Dot4_Credits( unit, packet[4] );
	}
}

// --

static void
Dot4_ReadIO( struct PrinterUnit *unit, U32 timeout )
{
	struct USB2_EPResource *resource;

	enum Dot4Stage stage;

	U8 *packet;
	U32 size;
	U32 length;

	resource = unit->unit_Bulk_Rx_Resource;
	packet = unit->unit_Dot4_RxBuffer;
	stage = unit->unit_Dot4_Stage;

	while ( unit->unit_Dot4_RxUsed >= DOT4_HEADER_SIZE )
	{
		length = Dot4_Word( &packet[2] );

		if ( ( length < DOT4_HEADER_SIZE ) || ( length > DOT4_MAX_PACKET ) )
		{
			Dot4_Fail( unit );

			return;
		}

		if ( unit->unit_Dot4_RxUsed < length )
		{
			break;
		}

		Dot4_Packet( unit, packet, length );

		// EXIT can clear the entire receive buffer.
		if ( unit->unit_Dot4_RxUsed >= length )
		{
			unit->unit_Dot4_RxUsed -= length;

			memmove( packet, packet + length, unit->unit_Dot4_RxUsed );
		}

		if ( ( stage != unit->unit_Dot4_Stage ) || ( unit->unit_Dot4_Action != DOT4_Receive ) )
		{
			return;
		}

		if ( ( stage == DOT4_Passive ) && ( unit->unit_Dot4_Credits ) )
		{
			unit->unit_Dot4_Stage = DOT4_Idle;

			return;
		}
	}

	size = resource->EndPointMax;

	if ( ( ! size ) || ( size > sizeof( unit->unit_Dot4_RxBuffer ) - unit->unit_Dot4_RxUsed ) )
	{
		Dot4_Fail( unit );

		return;
	}

	// A single endpoint packet completes even when it ends on a USB packet boundary.
	Dot4_Submit( unit, resource->IORequests[0], CMD_READ, packet + unit->unit_Dot4_RxUsed, size, timeout );
}

// --

static void
Dot4_PayloadIO( struct PrinterUnit *unit, U32 timeout )
{
	struct IOStdReq *request;

	U8 *packet;
	U32 length;

	request = unit->unit_Dot4_Request;
	packet = unit->unit_Dot4_TxBuffer;
	length = MIN( unit->unit_Dot4_WriteLength - request->io_Actual, unit->unit_Dot4_PacketSize - DOT4_HEADER_SIZE );

	Dot4_Header( packet, unit->unit_Dot4_Socket, length + DOT4_HEADER_SIZE, 0 );

	memcpy( &packet[DOT4_HEADER_SIZE], (U8 *)request->io_Data + request->io_Actual, length );

	Dot4_Submit( unit, unit->unit_Bulk_Tx_Resource->IORequests[0], CMD_WRITE, packet, length + DOT4_HEADER_SIZE, timeout );
}

// --

static void
Dot4_Progress( struct PrinterUnit *unit )
{
	U8 *packet;
	S32 remaining;
	U32 timeout;

	while ( ( unit->unit_Dot4_Stage != DOT4_Idle ) && ( ! unit->unit_Dot4_IO ) )
	{
		remaining = (S32)( unit->unit_Dot4_Deadline - Dot4_Now( unit ) );

		if ( remaining <= 0 )
		{
			if ( ( unit->unit_Dot4_Stage == DOT4_Passive ) && ( ! unit->unit_Dot4_RxUsed ) &&
				( unit->unit_Dot4_Action == DOT4_Receive ) )
			{
				unit->unit_Dot4_RequestCredit = TRUE;
				unit->unit_Dot4_Stage = DOT4_Idle;
			}
			else
			{
				Dot4_Fail( unit );
			}

			return;
		}

		timeout = MIN( (U32)remaining, DOT4_IO_US );

		switch ( unit->unit_Dot4_Action )
		{
			case DOT4_Control:
				Dot4_ControlIO( unit, timeout );
				break;

			case DOT4_Command:
				Dot4_CommandIO( unit, timeout );
				break;

			case DOT4_Receive:
				Dot4_ReadIO( unit, timeout );
				break;

			case DOT4_Acknowledge:
				packet = unit->unit_Dot4_TxBuffer;

				Dot4_Header( packet, 0, 10, 1 );

				packet[6] = DOT4_CMD_CREDIT_CHANNEL | 0x80;
				packet[7] = 0;
				packet[8] = unit->unit_Dot4_Socket;
				packet[9] = unit->unit_Dot4_Socket;

				Dot4_Submit( unit, unit->unit_Bulk_Tx_Resource->IORequests[0], CMD_WRITE, packet, 10, timeout );
				break;

			case DOT4_Payload:
				Dot4_PayloadIO( unit, timeout );
				break;
		}
	}
}

// --

void
Dot4_Complete( struct PrinterUnit *unit, struct USB2_IORequest *io )
{
	struct IOStdReq *request;

	S32 okay;

	if ( io != unit->unit_Dot4_IO )
	{
		return;
	}

	unit->unit_Dot4_IO = NULL;
	request = unit->unit_Dot4_Request;
	okay = ( ! io->io_Error ) && ( io->io_Actual == io->io_Length );

	if ( io->io_Error == USB2Err_Device_Detached )
	{
		unit->unit_Detached = TRUE;
	}

	if ( ( unit->unit_Dot4_Action == DOT4_Payload ) && ( okay ) )
	{
		request->io_Actual += io->io_Length - DOT4_HEADER_SIZE;
		unit->unit_Dot4_Credits--;
		unit->unit_Dot4_CreditTries = 0;
		unit->unit_Dot4_Stage = DOT4_Idle;
	}

	if ( ( request ) && ( ( request->io_Flags & IOPARF_ABORT ) || ( unit->unit_Dot4_Aborting ) ) )
	{
		if ( unit->unit_Dot4_Stage != DOT4_Idle )
		{
			unit->unit_Dot4_Dirty = TRUE;
			unit->unit_JobError = ParErr_LineErr;
		}

		Dot4_Finish( unit, unit->unit_JobError );

		return;
	}

	if ( ( unit->unit_Dot4_Action == DOT4_Payload ) && ( okay ) &&
		( request->io_Actual == unit->unit_Dot4_WriteLength ) )
	{
		Dot4_Finish( unit, 0 );

		return;
	}

	if ( unit->unit_Dot4_Action == DOT4_Receive )
	{
		if ( ( ! io->io_Error ) && ( io->io_Actual ) && ( io->io_Actual <= io->io_Length ) )
		{
			unit->unit_Dot4_RxUsed += io->io_Actual;
		}
		else if ( ( unit->unit_Dot4_Stage == DOT4_Passive ) && ( ! unit->unit_Dot4_RxUsed ) &&
			( ( io->io_Error == USB2Err_Host_Timeout ) || ( ! io->io_Error ) ) )
		{
			unit->unit_Dot4_RequestCredit = TRUE;
			unit->unit_Dot4_Stage = DOT4_Idle;
		}
		else
		{
			Dot4_Fail( unit );
		}

		return;
	}

	if ( ! okay )
	{
		Dot4_Fail( unit );

		return;
	}

	switch ( unit->unit_Dot4_Action )
	{
		case DOT4_Control:
			if ( unit->unit_Dot4_Stage == DOT4_Reset )
			{
				Dot4_ClearSession( unit );

				Dot4_Begin( unit, DOT4_ClearOut );
			}
			else if ( unit->unit_Dot4_Stage == DOT4_ClearOut )
			{
				Dot4_Begin( unit, DOT4_ClearIn );
			}
			else
			{
				Dot4_Begin( unit, DOT4_Init );
			}
			break;

		case DOT4_Acknowledge:
			if ( ( unit->unit_Dot4_Stage == DOT4_Passive ) && ( unit->unit_Dot4_Credits ) )
			{
				unit->unit_Dot4_Stage = DOT4_Idle;
			}
			else
			{
				unit->unit_Dot4_Action = DOT4_Receive;
			}
			break;

		case DOT4_Command:
			unit->unit_Dot4_Action = DOT4_Receive;
			break;

		default:
			break;
	}
}

// --

void
Dot4_Abort( struct PrinterUnit *unit )
{
	struct IOStdReq *request;

	request = unit->unit_Dot4_Request;

	if ( ( ! request ) || ( ( ! unit->unit_JobError ) && ( ! ( request->io_Flags & IOPARF_ABORT ) ) ) )
	{
		return;
	}

	if ( unit->unit_Dot4_IO )
	{
		if ( ! unit->unit_Dot4_Aborting )
		{
			unit->unit_Dot4_Aborting = TRUE;

			AbortIO( (PTR)unit->unit_Dot4_IO );
		}
	}
	else
	{
		if ( unit->unit_Dot4_Stage != DOT4_Idle )
		{
			unit->unit_Dot4_Dirty = TRUE;
			unit->unit_JobError = ParErr_LineErr;
		}

		Printer_TimerStop( &unit->unit_IdleTimer );

		Dot4_Finish( unit, unit->unit_JobError );
	}
}

// --

void
Dot4_Drain( struct PrinterUnit *unit )
{
	struct USB2_IORequest *io;

	io = unit->unit_Dot4_IO;

	if ( io )
	{
		unit->unit_Dot4_Aborting = TRUE;

		AbortIO( (PTR)io );

		WaitIO( (PTR)io );

		if ( ! unit->unit_JobError )
		{
			unit->unit_JobError = ParErr_NotOpen;
		}

		Dot4_Complete( unit, io );
	}

	if ( unit->unit_Dot4_Request )
	{
		Dot4_Finish( unit, ParErr_NotOpen );
	}
}

// --

void
Dot4_Schedule( struct PrinterUnit *unit )
{
	struct IOStdReq *request;

	U32 index;

	// Bound work on a queue of empty or already-aborted requests.
	for ( index = 0 ; index < 32 ; index++ )
	{
		if ( ( unit->unit_Detached ) || ( unit->unit_Dot4_IO ) || ( unit->unit_IdleTimer.Pending ) ||
			( unit->unit_JobError ) )
		{
			return;
		}

		request = unit->unit_Dot4_Request;

		if ( ! request )
		{
			request = (PTR)RemHead( unit->unit_PAR_IOReq_WaitingList );

			if ( ! request )
			{
				return;
			}

			unit->unit_PAR_IOReq_WaitingCount--;
			unit->unit_Dot4_Request = request;
			unit->unit_Dot4_CreditTries = 0;

			unit->unit_Dot4_WriteLength = ( request->io_Length == -1U ) ?
				strlen( request->io_Data ) : request->io_Length;
		}

		if ( request->io_Flags & IOPARF_ABORT )
		{
			Dot4_Abort( unit );

			continue;
		}

		if ( unit->unit_Dot4_Stage == DOT4_Idle )
		{
			if ( request->io_Actual == unit->unit_Dot4_WriteLength )
			{
				Dot4_Finish( unit, 0 );

				continue;
			}

			if ( unit->unit_Dot4_Dirty )
			{
				Dot4_Begin( unit, DOT4_Reset );
			}
			else if ( unit->unit_Dot4_RxUsed )
			{
				Dot4_Begin( unit, DOT4_Passive );
			}
			else if ( unit->unit_Dot4_Credits )
			{
				Dot4_Begin( unit, DOT4_SendData );
			}
			else if ( ! unit->unit_Dot4_RequestCredit )
			{
				Dot4_Begin( unit, DOT4_Passive );
			}
			else if ( unit->unit_Dot4_CreditTries < DOT4_CREDIT_TRIES )
			{
				unit->unit_Dot4_CreditTries++;

				Dot4_Begin( unit, DOT4_Credit );
			}
			else
			{
				Dot4_Fail( unit );

				return;
			}
		}

		Dot4_Progress( unit );
	}

	Signal( FindTask( NULL ), unit->unit_PRT_Begin_MsgPortBit );
}

// --

static void
Dot4_RunSetup( struct PrinterUnit *unit, enum Dot4Stage stage )
{
	struct USB2_IORequest *io;

	Dot4_Begin( unit, stage );

	while ( ( unit->unit_Dot4_Stage != DOT4_Idle ) && ( ! unit->unit_Detached ) )
	{
		Dot4_Progress( unit );

		io = unit->unit_Dot4_IO;

		if ( io )
		{
			WaitIO( (PTR)io );

			Dot4_Complete( unit, io );
		}
	}
}

// --

S32
Dot4_Setup( struct PrinterUnit *unit )
{
	Dot4_RunSetup( unit, DOT4_Init );

	return ( unit->unit_Dot4_Ready );
}

// --

void
Dot4_Shutdown( struct PrinterUnit *unit )
{
	if ( unit->unit_Dot4_ChannelOpen )
	{
		Dot4_RunSetup( unit, DOT4_Close );
	}

	if ( ( unit->unit_Dot4_Initialized ) && ( ! unit->unit_Detached ) )
	{
		Dot4_RunSetup( unit, DOT4_Exit );
	}

	Dot4_ClearSession( unit );
}

// --
