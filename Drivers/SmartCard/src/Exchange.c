
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/SmartCard.h"

// --

LONG
SmartCard_Exchange( struct SmartCardReader *reader, struct SmartCardCommand *command )
{
	struct USB2SmartCardReply *reply;
	struct USB2_EPResource *endpoint;

	static const U8 opcodes[] = { 0x65, 0x62, 0x63, 0x6f };
	U8 *buffer;
	U32 size;
	U32 actual;
	U32 length;
	U32 start;
	S32 usbError;
	LONG result;
	U8 expected;
	U8 sequence;
	U8 slot;
	BOOL abortable;

	size = command->InputSize;
	slot = command->Slot;
	reply = &command->Reply;

	if ( size > reader->Info.MaxMessage - CCID_HEADER )
	{
		return ( USB2SC_BadArgument );
	}

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
	abortable = ( command->Operation == USB2SCC_PowerOn ) || ( command->Operation == USB2SCC_Exchange );
	expected = abortable ? 0x80 : 0x81;
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

	start = SmartCard_Now( reader );
	endpoint = reader->Out;

	usbError = SmartCard_Transfer( reader, endpoint, CMD_WRITE, buffer, CCID_HEADER + size,
		start, CCID_COMMAND_US, CCID_IO_US, &actual );

	if ( usbError == USB2Err_Device_Detached )
	{
		return ( USB2SC_NoReader );
	}

	if ( ( usbError ) || ( actual != CCID_HEADER + size ) )
	{
		if ( usbError == USB2Err_Stack_NoMemory )
		{
			return ( USB2SC_NoMemory );
		}

		if ( ( usbError == USB2Err_Host_Stall ) && ( ! actual ) )
		{
			// The endpoint rejected the command before accepting any data.
			if ( reader->USB->USB2_EPRes_Destall( endpoint ) )
			{
				return ( USB2SC_USBError );
			}

			reader->Faulted = TRUE;

			IExec->DebugPrintF( "USB2 CCID: bulk OUT remains halted, reconnect reader\n" );

			return ( USB2SC_USBError );
		}

		result = USB2SC_USBError;

		goto recover;
	}

	endpoint = reader->In;

	result = SmartCard_Receive( reader, slot, sequence, expected,
		start, CCID_COMMAND_US, FALSE, &length, &usbError );

	if ( ( result == USB2SC_NoReader ) || ( result == USB2SC_USBError ) || ( result == USB2SC_ProtocolError ) )
	{
		goto recover;
	}

	reply->State = buffer[7] & 0x03;
	reply->ReaderError = ( result == USB2SC_ReaderError ) ? buffer[8] : 0;

	if ( result == USB2SC_ReaderError )
	{
		return ( result );
	}

	if ( ( expected == 0x80 ) && ( buffer[9] ) )
	{
		result = USB2SC_Unsupported;

		goto recover;
	}

	if ( ( ( expected == 0x81 ) && ( length ) ) ||
		( ( command->Operation == USB2SCC_PowerOn ) && ( ( ! length ) || ( length > USB2SMARTCARD_ATR_MAX ) ) ) ||
		( ( command->Operation == USB2SCC_Exchange ) && ( ( length < 2 ) || ( length > USB2SMARTCARD_APDU_RESPONSE_MAX ) ) ) )
	{
		// The final reply consumed the command even when its payload is invalid.
		return ( USB2SC_ProtocolError );
	}

	reply->Length = length;

	if ( length )
	{
		IExec->CopyMem( buffer + CCID_HEADER, command->Output, length );
	}

	return ( USB2SC_Okay );

recover:
	if ( result != USB2SC_NoReader )
	{
		IExec->DebugPrintF( "USB2 CCID: slot %lu sequence %lu command $%02lx, result %ld USB %ld\n",
			(U32)slot, (U32)sequence, (U32)opcodes[command->Operation], result, usbError );

		if ( ( ! SmartCard_Recover( reader, endpoint, usbError, slot, sequence, expected, abortable ) ) &&
			( ! reader->Stopped ) )
		{
			reader->Faulted = TRUE;

			IExec->DebugPrintF( "USB2 CCID: recovery failed for slot %lu, reconnect reader\n", (U32)slot );
		}
	}

	return ( reader->Stopped ? USB2SC_NoReader : result );
}

// --
