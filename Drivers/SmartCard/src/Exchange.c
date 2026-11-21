
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/SmartCard.h"

// --

/*
 * CCID 1.1 bulk messages share a ten-byte header. Keep every transaction
 * on the reader worker, including time-extension replies.
 */
static LONG
Transfer( struct USB2_EPResource *endpoint, U16 operation, U8 *buffer, U32 length, U32 *actual )
{
	struct USB2_IORequest *request;

	request = endpoint->IORequests[0];
	request->io_Command = operation;
	request->io_Data = buffer;
	request->io_Length = length;
	request->io_TimeOut = 2000000;

	IExec->DoIO( (PTR) request );

	*actual = request->io_Actual;

	if ( request->io_Error == USB2Err_Device_Detached )
	{
		return ( USB2SC_NoReader );
	}

	return ( request->io_Error ? USB2SC_USBError : USB2SC_Okay );
}

// --

LONG
SmartCard_Exchange( struct SmartCardReader *reader, struct SmartCardCommand *command )
{
	struct USB2SmartCardReply *reply;

	static const U8 opcodes[] = { 0x65, 0x62, 0x63, 0x6f };

	U8 *buffer;
	U32 size;
	U32 actual;
	U32 length;
	U32 attempt;
	LONG result;
	U8 expected;
	U8 sequence;
	U8 status;
	U8 slot;

	size = command->InputSize;
	slot = command->Slot;
	reply = command->Reply;

	if ( size > reader->Info.MaxMessage - CCID_HEADER )
	{
		return ( USB2SC_BadArgument );
	}

	// Only power on cards when the reader can choose their voltage.
	if ( ( command->Operation == USB2SCC_PowerOn ) && ( ! ( reader->Info.Features & 0x00000008UL ) ) )
	{
		return ( USB2SC_Unsupported );
	}

	if ( ( command->Operation == USB2SCC_Exchange ) &&
		( ! ( reader->Info.Features & ( USB2SMARTCARD_SHORT_APDU | USB2SMARTCARD_EXTENDED_APDU ) ) ) )
	{
		return ( USB2SC_Unsupported );
	}

	buffer = reader->Buffer;
	sequence = reader->Sequence++;
	expected = ( ( command->Operation == USB2SCC_PowerOn ) || ( command->Operation == USB2SCC_Exchange ) ) ? 0x80 : 0x81;
	buffer[0] = opcodes[command->Operation];
	buffer[1] = (U8) size;
	buffer[2] = (U8) ( size >> 8 );
	buffer[3] = 0;
	buffer[4] = 0;
	buffer[5] = slot;
	buffer[6] = sequence;
	buffer[7] = 0;
	buffer[8] = 0;
	buffer[9] = 0;

	if ( size )
	{
		IExec->CopyMem( command->Input, buffer + CCID_HEADER, size );
	}

	result = Transfer( reader->Out, CMD_WRITE, buffer, CCID_HEADER + size, & actual );

	if ( result != USB2SC_Okay )
	{
		return ( result );
	}

	if ( actual != CCID_HEADER + size )
	{
		return ( USB2SC_USBError );
	}

	for( attempt = 0; attempt < 16; attempt++ )
	{
		result = Transfer( reader->In, CMD_READ, buffer, reader->Info.MaxMessage, & actual );

		if ( result != USB2SC_Okay )
		{
			return ( result );
		}

		if ( ( actual < CCID_HEADER ) || ( actual > reader->Info.MaxMessage ) )
		{
			return ( USB2SC_ProtocolError );
		}

		length = CCID_Read32( buffer + 1 );

		if ( length != actual - CCID_HEADER )
		{
			return ( USB2SC_ProtocolError );
		}

		if ( ( buffer[5] != slot ) || ( buffer[6] != sequence ) )
		{
			// A reply from an earlier timed-out command can still be queued.
			continue;
		}

		status = buffer[7] >> 6;

		if ( ( buffer[0] != expected ) || ( ( buffer[7] & 0x03 ) == 3 ) || ( status == 3 ) )
		{
			return ( USB2SC_ProtocolError );
		}

		if ( status == 2 )
		{
			continue;
		}

		reply->State = buffer[7] & 0x03;
		reply->ReaderError = buffer[8];

		if ( status == 1 )
		{
			return ( USB2SC_ReaderError );
		}

		if ( ( expected == 0x80 ) && ( buffer[9] ) )
		{
			return ( USB2SC_Unsupported );
		}

		if ( ( ( expected == 0x81 ) && ( length ) ) ||
			( ( command->Operation == USB2SCC_PowerOn ) && ( ( ! length ) || ( length > USB2SMARTCARD_ATR_MAX ) ) ) ||
			( ( command->Operation == USB2SCC_Exchange ) && ( ( length < 2 ) || ( length > USB2SMARTCARD_APDU_RESPONSE_MAX ) ) ) )
		{
			return ( USB2SC_ProtocolError );
		}

		reply->Length = length;

		if ( length > command->OutputSize )
		{
			return ( USB2SC_BufferTooSmall );
		}

		if ( length )
		{
			IExec->CopyMem( buffer + CCID_HEADER, command->Output, length );
		}

		return ( USB2SC_Okay );
	}

	return ( USB2SC_USBError );
}

// --
