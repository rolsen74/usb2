
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Radio.h"
#include "Registers.h"

// --

S32
RTL8821CU_SetAssociation( struct EthernetUnit *unit, U8 *bssid, U16 aid, S32 receiveData )
{
	struct USB2_EPResource *control;

	control = unit->unit_USB_Register->Res_Control;

	if ( ( ! RTL8821CU_RegisterIO( control, CMD_WRITE, 0x0618, bssid, RTL8821CU_ADDR_SIZE ) ) ||
		( ! RTL8821CU_WriteReg( control, 0x06a8, 2, aid ? aid | 0xc000 : 0 ) ) ||
		( ! RTL8821CU_UpdateReg( control, 0x0102, 1, 3, aid ? 2 : 0 ) ) ||
		( ! RTL8821CU_WriteReg( control, 0x06a4, 2, receiveData ? 0xffff : 0 ) ) ||
		( ! RTL8821CU_UpdateReg( control, 0x0608, 4, 0x40, receiveData ? 0x40 : 0 ) ) )
	{
		return ( FALSE );
	}

	memcpy( unit->unit_BSSID, bssid, RTL8821CU_ADDR_SIZE );

	unit->unit_AssocID = aid;
	unit->unit_BeaconAge = 0;
	unit->unit_BeaconLimit = RTL8821CU_BEACON_TIMEOUT;

	return ( TRUE );
}

// --
