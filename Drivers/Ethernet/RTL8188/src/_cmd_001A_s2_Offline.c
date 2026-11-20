
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Firmware.h"
#include "Commands.h"
#include "Radio.h"

// --

void
Cmd_Offline( struct EthernetUnit *unit, struct IOSana2Req *ioreq UNUSED )
{
	USBINFO( "RTL8188EU: S2_OFFLINE, AID %lu, online %lu", (U32) unit->unit_AssocID, (U32) unit->unit_Online );

	if ( unit->unit_Online )
	{
		RTL8188EU_DropLink( unit );
	}
}

// --
