
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Radio.h"

// --

S32
RTL8811AU_SetAssociation( struct EthernetUnit *unit, U8 *bssid, U16 aid )
{
	struct USB2_EPResource *control;

	control = unit->unit_Control;

	if ( ( ! RTL8811AU_RegisterIO( control, CMD_WRITE, 0x0618, bssid, RTL8811AU_MAC_SIZE ) ) ||
		( ! RTL8811AU_WriteReg( control, 0x06a8, 2, aid ? aid | 0xc000 : 0 ) ) ||
		( ! RTL8811AU_Update8( control, 0x0102, 3, aid ? 2 : 0 ) ) ||
		( ! RTL8811AU_WriteReg( control, 0x06a4, 2, aid ? 0xffff : 0 ) ) ||
		( ! RTL8811AU_UpdateReg( control, 0x0608, 4, 0x40, aid ? 0x40 : 0 ) ) )
	{
		return ( FALSE );
	}

	memcpy( unit->unit_BSSID, bssid, RTL8811AU_MAC_SIZE );

	unit->unit_AssocID = aid;

	return ( TRUE );
}

// --
