
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
Process_Free( struct EthernetUnit *unit )
{
	struct Message *msg;

	S32 detached;

	ObtainSemaphore( & unit->unit_CommandLock );

	detached = unit->unit_Detached;
	unit->unit_Detached = TRUE;

	AbortPendingCommands( unit, NULL, NULL );

	ReleaseSemaphore( & unit->unit_CommandLock );

	RT2870_StopBeacon( unit, detached );

	if ( unit->unit_BulkOut )
	{
		USB2_EPRes_Release( unit->unit_BulkOut );

		unit->unit_BulkOut = NULL;
	}

	if ( unit->unit_USB_Register )
	{
		while(( msg = GetMsg( unit->unit_USB_Register->Stack_MsgPort ) ))
		{
			ReplyMsg( msg );
		}

		USB2_Reg_Unregister( unit->unit_USB_Register );
	}
}

// --
