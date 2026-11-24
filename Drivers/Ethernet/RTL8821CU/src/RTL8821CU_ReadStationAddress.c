
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define EFUSE_MAP_SIZE 512
#define EFUSE_MAC 0x107
#define EFUSE_XTAL 0xb9
#define EFUSE_SWING_2GHZ 0xc6
#define EFUSE_RF_OPTION 0xca
#define EFUSE_TX_POWER_A 0x10
#define EFUSE_TX_POWER_B 0x3a

// --

S32
RTL8821CU_ReadStationAddress( struct EthernetUnit *unit )
{
	U8 map[EFUSE_MAP_SIZE];
	U32 power;

	if ( ! RTL8821CU_ReadEFuse( unit->unit_USB_Register->Res_Control, map, sizeof( map ) ) )
	{
		return ( FALSE );
	}

	if ( ( map[0] != 0x29 ) || ( map[1] != 0x81 ) || ( ! ValidStationAddress( map + EFUSE_MAC ) ) )
	{
		DebugPrintF( "RTL8821CU: invalid Wi-Fi eFuse map or station address\n" );

		return ( FALSE );
	}

	unit->unit_RFOption = map[EFUSE_RF_OPTION] == 0xff ? 0 : map[EFUSE_RF_OPTION];
	unit->unit_CrystalCap = map[EFUSE_XTAL] == 0xff ? 0x20 : map[EFUSE_XTAL] & 0x3f;
	unit->unit_BBSwing = map[EFUSE_SWING_2GHZ] > 9 ? 0 : map[EFUSE_SWING_2GHZ];
	power = ( ( ( unit->unit_RFOption & 0x1f ) == 2 ) || ( ( unit->unit_RFOption & 0x1f ) == 4 ) ) ?
		EFUSE_TX_POWER_B : EFUSE_TX_POWER_A;

	CopyMem( map + EFUSE_MAC, unit->unit_StationAddress, RTL8821CU_ADDR_SIZE );

	CopyMem( map + power, unit->unit_CCKPower, sizeof( unit->unit_CCKPower ) );

	DebugPrintF( "RTL8821CU: station address %02lx:%02lx:%02lx:%02lx:%02lx:%02lx\n",
		(U32) unit->unit_StationAddress[0], (U32) unit->unit_StationAddress[1], (U32) unit->unit_StationAddress[2],
		(U32) unit->unit_StationAddress[3], (U32) unit->unit_StationAddress[4], (U32) unit->unit_StationAddress[5] );

	DebugPrintF( "RTL8821CU: RF option $%02lx, crystal cap $%02lx\n",
		(U32) unit->unit_RFOption, (U32) unit->unit_CrystalCap );

	return ( TRUE );
}

// --
