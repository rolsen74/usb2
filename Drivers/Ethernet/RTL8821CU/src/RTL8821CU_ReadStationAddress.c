
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define RTL8821CU_MACID 0x0610

// --

S32
RTL8821CU_ReadStationAddress( struct EthernetUnit *unit )
{
	struct USB2_EPResource *control;

	U8 *address;

	control = unit->unit_USB_Register->Res_Control;
	address = unit->unit_StationAddress;

	if ( ( ! RTL8821CU_RegisterIO( control, CMD_READ, RTL8821CU_MACID, address, 4 ) ) ||
		( ! RTL8821CU_RegisterIO( control, CMD_READ, RTL8821CU_MACID + 4, address + 4, 2 ) ) ||
		( ! ValidStationAddress( address ) ) )
	{
		return ( RTL8821CU_ReadEFuseAddress( control, address ) );
	}

	return ( TRUE );
}

// --
