
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
SmartCard_Receive( struct SmartCardReader *reader, U8 slot, U8 sequence, U8 expected,
	U32 start, U32 budget, BOOL recovering, U32 *length, S32 *usbError )
{
	U8 *buffer;
	U8 status;
	U32 actual;
	U32 window;
	U32 now;
	U32 timeout;

	buffer = reader->Buffer;
	timeout = CCID_IO_US;
	window = SmartCard_Now( reader );

	for( ;; )
	{
		now = SmartCard_Now( reader );

		if ( now - window >= timeout )
		{
			*usbError = USB2Err_Host_Timeout;

			return ( USB2SC_USBError );
		}

		*usbError = SmartCard_Transfer( reader, reader->In, CMD_READ, buffer, reader->ReceiveSize,
			start, budget, timeout - ( now - window ), &actual );

		if ( *usbError )
		{
			return ( ( *usbError == USB2Err_Device_Detached ) ? USB2SC_NoReader : USB2SC_USBError );
		}

		if ( ( actual < CCID_HEADER ) || ( actual > reader->Info.MaxMessage ) )
		{
			// An aborted reply can end with an incomplete frame.
			if ( recovering )
			{
				continue;
			}

			return ( USB2SC_ProtocolError );
		}

		if ( ( buffer[5] != slot ) || ( buffer[6] != sequence ) )
		{
			continue;
		}

		*length = CCID_Read32( buffer + 1 );
		status = buffer[7] >> 6;

		if ( ( buffer[0] != expected ) || ( ( buffer[7] & 0x03 ) == 3 ) ||
			( status == 3 ) || ( *length != actual - CCID_HEADER ) )
		{
			return ( USB2SC_ProtocolError );
		}

		if ( status == 2 )
		{
			// APDU readers manage card timing internally. Use a bounded
			// estimate when the negotiated wait time is unavailable.
			timeout = (U32) buffer[8] * CCID_TIME_UNIT_US;

			if ( timeout < CCID_IO_US )
			{
				timeout = CCID_IO_US;
			}

			window = SmartCard_Now( reader );

			continue;
		}

		return ( ( status == 1 ) ? USB2SC_ReaderError : USB2SC_Okay );
	}
}

// --
