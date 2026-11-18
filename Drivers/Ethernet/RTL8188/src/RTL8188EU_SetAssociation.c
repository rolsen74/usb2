
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Firmware.h"
#include "Radio.h"

// --

S32
RTL8188EU_SetAssociation( struct EthernetUnit *unit, U8 *bssid, U16 aid )
{
	struct USB2_EPResource *control;

	control = unit->unit_USB_Register->Res_Control;

	if ( ( ! RTL8188EU_RegisterIO( control, CMD_WRITE, 0x0618, bssid, 6 ) ) ||
		( ! RTL8188EU_Write16( control, 0x06a8, aid ? aid | 0xc000 : 0 ) ) ||
		( ! RTL8188EU_Update8( control, 0x0102, 3, aid ? 2 : 0 ) ) ||
		( ! RTL8188EU_Write16( control, 0x06a4, aid ? 0xffff : 0 ) ) )
	{
		return ( FALSE );
	}

	memcpy( unit->unit_BSSID, bssid, 6 );
	unit->unit_AssocID = aid;

	return ( TRUE );
}

// --
