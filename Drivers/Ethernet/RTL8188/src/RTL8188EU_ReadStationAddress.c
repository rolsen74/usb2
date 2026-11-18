
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Firmware.h"

// --

#define RTL8188EU_MACID 0x0610
#define RTL8188EU_EFUSE_MACID 0xd7

// --

S32
RTL8188EU_ReadStationAddress( struct EthernetUnit *unit )
{
	struct USB2_EPResource *control;
	static const U8 zero[6] = { 0 };

	U8 rom[256];
	U8 *address;

	control = unit->unit_USB_Register->Res_Control;
	address = unit->unit_StationAddress;
	unit->unit_CrystalCap = 0x20;

	memset( unit->unit_CCKPower, 0xff, sizeof( unit->unit_CCKPower ) );

	if ( RTL8188EU_ReadEFuse( control, rom, sizeof( rom ) ) )
	{
		if ( rom[0xb9] != 0xff )
		{
			unit->unit_CrystalCap = rom[0xb9] & 0x3f;
		}

		memcpy( unit->unit_CCKPower, rom + 0x10, sizeof( unit->unit_CCKPower ) );

		memcpy( address, rom + RTL8188EU_EFUSE_MACID, sizeof( zero ) );

		if ( ( ! ( address[0] & 1 ) ) && ( memcmp( address, zero, sizeof( zero ) ) ) )
		{
			return ( TRUE );
		}
	}

	return ( RTL8188EU_RegisterIO( control, CMD_READ, RTL8188EU_MACID, address, sizeof( zero ) ) &&
		( ! ( address[0] & 1 ) ) && ( memcmp( address, zero, sizeof( zero ) ) ) );
}

// --
