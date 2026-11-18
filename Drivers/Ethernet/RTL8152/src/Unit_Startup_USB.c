
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

int
unit_Process_OpenUSBDevice( struct devUnit *unit )
{
	unit->unit_USBPort = IExec->AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! unit->unit_USBPort )
	{
		return ( FALSE );
	}

	unit->unit_USBReq = IExec->AllocSysObjectTags( ASOT_IOREQUEST,
		ASOIOR_Size, sizeof( struct IORequest ),
		ASOIOR_ReplyPort, unit->unit_USBPort,
		TAG_END );

	if ( ! unit->unit_USBReq )
	{
		return ( FALSE );
	}

	if ( IExec->OpenDevice( "usb2.device", 0, unit->unit_USBReq, 0 ) )
	{
		return ( FALSE );
	}

	unit->unit_USBDevice = unit->unit_USBReq->io_Device;

	unit->unit_IUSB2 = (PTR) IExec->GetInterface( (PTR) unit->unit_USBDevice, "main", 1, NULL );

	return ( unit->unit_IUSB2 != NULL );
}

// --
