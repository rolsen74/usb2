
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

S32
BTUSB_OpenTimer( struct BTUSB *usb )
{
	usb->TimerPort = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! usb->TimerPort )
	{
		return ( FALSE );
	}

	usb->TimerRequest = AllocSysObjectTags( ASOT_IOREQUEST, ASOIOR_Size, sizeof( *usb->TimerRequest ),
		ASOIOR_ReplyPort, usb->TimerPort, TAG_END );

	if ( ! usb->TimerRequest )
	{
		return ( FALSE );
	}

	if ( OpenDevice( TIMERNAME, UNIT_MICROHZ, & usb->TimerRequest->Request, 0 ) )
	{
		usb->TimerRequest->Request.io_Device = NULL;

		return ( FALSE );
	}

	usb->Timer = (PTR) GetInterface( (PTR) usb->TimerRequest->Request.io_Device, "main", 1, NULL );

	return ( usb->Timer != NULL );
}

// --

void
BTUSB_CloseTimer( struct BTUSB *usb )
{
	if ( usb->Timer )
	{
		DropInterface( (PTR) usb->Timer );
	}

	if ( usb->TimerRequest )
	{
		if ( usb->TimerRequest->Request.io_Device )
		{
			CloseDevice( & usb->TimerRequest->Request );
		}

		FreeSysObject( ASOT_IOREQUEST, usb->TimerRequest );
	}

	if ( usb->TimerPort )
	{
		FreeSysObject( ASOT_PORT, usb->TimerPort );
	}
}

// --

U64
BTUSB_Now( struct BTUSB *usb )
{
	struct TimeVal now;

	usb->Timer->GetUpTime( & now );

	return ( (U64) now.Seconds * 1000000U + now.Microseconds );
}

// --
