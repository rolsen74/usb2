
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/SmartCard.h"

// --

static BOOL
SendAbort( struct SmartCardReader *reader, U8 slot, U8 sequence, U32 start )
{
	struct USB2_EPResource *control;
	struct USB2_IORequest *request;
	struct USB2_SetupData *setup;

	U8 *buffer;
	U32 actual;
	S32 error;

	control = reader->Registration->Res_Control;
	request = control->IORequests[0];
	setup = control->SetupData;
	setup->RequestType = REQTYPE_Write | REQTYPE_Class | REQTYPE_Interface;
	setup->RequestCode = 1;
	// USB setup fields are little-endian on the wire.
	((U8 *) &setup->Value)[0] = slot;
	((U8 *) &setup->Value)[1] = sequence;
	((U8 *) &setup->Index)[0] = reader->Interface;
	((U8 *) &setup->Index)[1] = 0;
	setup->Length = 0;
	request->io_SetupData = setup;
	request->io_SetupLength = sizeof( *setup );

	error = SmartCard_Transfer( reader, control, CMD_WRITE, NULL, 0,
		start, CCID_RECOVERY_US, CCID_IO_US, &actual );

	if ( error )
	{
		return ( FALSE );
	}

	buffer = reader->Buffer;
	buffer[0] = 0x72;
	buffer[1] = 0;
	buffer[2] = 0;
	buffer[3] = 0;
	buffer[4] = 0;
	buffer[5] = slot;
	buffer[6] = sequence;
	buffer[7] = 0;
	buffer[8] = 0;
	buffer[9] = 0;

	error = SmartCard_Transfer( reader, reader->Out, CMD_WRITE, buffer, CCID_HEADER,
		start, CCID_RECOVERY_US, CCID_IO_US, &actual );

	return ( ( ! error ) && ( actual == CCID_HEADER ) );
}

// --

BOOL
SmartCard_Recover( struct SmartCardReader *reader, struct USB2_EPResource *endpoint,
	S32 usbError, U8 slot, U8 sequence, U8 expected, BOOL abortable )
{
	U32 start;
	U32 length;
	LONG result;

	if ( reader->Stopped )
	{
		return ( FALSE );
	}

	start = SmartCard_Now( reader );

	if ( ( usbError == USB2Err_Host_Stall ) &&
		( ! reader->USB->USB2_EPRes_Destall( endpoint ) ) )
	{
		return ( FALSE );
	}

	if ( abortable )
	{
		// The ABORT pair uses its own sequence, separate from the old reply.
		sequence = reader->Sequence++;

		if ( ! SendAbort( reader, slot, sequence, start ) )
		{
			return ( FALSE );
		}

		expected = 0x81;
	}

	result = SmartCard_Receive( reader, slot, sequence, expected,
		start, CCID_RECOVERY_US, TRUE, &length, &usbError );

	if ( abortable )
	{
		return ( ( result == USB2SC_Okay ) && ( ! length ) );
	}

	return ( ( ( result == USB2SC_Okay ) || ( result == USB2SC_ReaderError ) ) && ( ! length ) );
}

// --
