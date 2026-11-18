
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

void
unit_Process_CloseUSBDevice( struct devUnit *unit )
{
	if ( unit->unit_IUSB2 )
	{
		IExec->DropInterface( (PTR) unit->unit_IUSB2 );
	}

	if ( unit->unit_USBDevice )
	{
		IExec->CloseDevice( unit->unit_USBReq );
	}

	if ( unit->unit_USBReq )
	{
		IExec->FreeSysObject( ASOT_IOREQUEST, unit->unit_USBReq );
	}

	if ( unit->unit_USBPort )
	{
		IExec->FreeSysObject( ASOT_PORT, unit->unit_USBPort );
	}
}

// --
