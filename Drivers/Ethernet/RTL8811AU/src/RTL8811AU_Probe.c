
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

#define EFUSE_MAP_SIZE 512
#define EFUSE_CCK_POWER 0x010
#define EFUSE_CRYSTAL 0x0b9
#define EFUSE_PA_TYPE 0x0bc
#define EFUSE_LNA_2GHZ 0x0bd
#define EFUSE_LNA_5GHZ 0x0bf
#define EFUSE_MAC_OFFSET 0x107
#define MULTI_FUNC_CTRL 0x0068
#define MULTI_BT_ENABLE 0x00040000

// --

S32
RTL8811AU_Probe( struct USB2_EPResource *control, U8 *station, struct RTL8811AU_Board *board )
{
	const U8 *address;
	U32 config;
	U32 index;
	U8 bits;
	U8 pa;
	U8 lna_2ghz;
	U8 lna_5ghz;
	U8 rom[EFUSE_MAP_SIZE];

	if ( ! RTL8811AU_ReadReg( control, RTL8811AU_SYS_CFG, 4, &config ) )
	{
		return ( FALSE );
	}

	if ( config == 0xffffffffU )
	{
		DebugPrintF( "RTL8811AU: invalid SYS_CFG\n" );

		return ( FALSE );
	}

	DebugPrintF( "RTL8811AU: SYS_CFG $%08lx, chip cut %lu\n", config, ( config >> 12 ) & 0x0f );

	if ( ! RTL8811AU_ReadEFuse( control, rom, sizeof( rom ) ) )
	{
		DebugPrintF( "RTL8811AU: eFuse read failed\n" );

		return ( FALSE );
	}

	address = &rom[EFUSE_MAC_OFFSET];
	bits = 0;

	for( index = 0; index < RTL8811AU_MAC_SIZE; index++ )
	{
		bits |= address[index];
	}

	if ( ( ! bits ) || ( address[0] & 1 ) )
	{
		DebugPrintF( "RTL8811AU: invalid eFuse station address\n" );

		return ( FALSE );
	}

	DebugPrintF( "RTL8811AU: station address %02lx:%02lx:%02lx:%02lx:%02lx:%02lx\n",
		(U32) address[0], (U32) address[1], (U32) address[2],
		(U32) address[3], (U32) address[4], (U32) address[5] );

	CopyMem( (PTR) address, station, RTL8811AU_MAC_SIZE );

	if ( ! RTL8811AU_ReadReg( control, MULTI_FUNC_CTRL, 4, &config ) )
	{
		return ( FALSE );
	}

	pa = rom[EFUSE_PA_TYPE] == 0xff ? 0 : rom[EFUSE_PA_TYPE];
	lna_2ghz = rom[EFUSE_LNA_2GHZ] == 0xff ? 0 : rom[EFUSE_LNA_2GHZ];
	lna_5ghz = rom[EFUSE_LNA_5GHZ] == 0xff ? 0 : rom[EFUSE_LNA_5GHZ];
	board->CrystalCap = rom[EFUSE_CRYSTAL] == 0xff ? 0x20 : ( rom[EFUSE_CRYSTAL] & 0x3f );
	board->ExternalLNA2GHz = ( lna_2ghz & 0x08 ) != 0;
	board->BoardType = ( pa & 0x01 ? RTL8811AU_EXT_PA_5GHZ : 0 ) |
		( lna_5ghz & 0x08 ? RTL8811AU_EXT_LNA_5GHZ : 0 ) |
		( config & MULTI_BT_ENABLE ? RTL8811AU_BOARD_BT : 0 );

	CopyMem( & rom[EFUSE_CCK_POWER], board->CCKPower, sizeof( board->CCKPower ) );

	if ( ( ! board->BoardType ) && ( ! board->ExternalLNA2GHz ) && ( ! ( pa & 0x10 ) ) )
	{
		board->BoardType = RTL8811AU_BOARD_DEFAULT;
	}

	DebugPrintF( "RTL8811AU: RF board flags $%02lx, crystal cap $%02lx, external 2 GHz LNA %lu\n",
		(U32) board->BoardType, (U32) board->CrystalCap, (U32) board->ExternalLNA2GHz );

	return ( TRUE );
}

// --
