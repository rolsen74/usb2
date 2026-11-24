
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define RTL8821CU_SYS_CFG1 0x00f0
#define RTL8821CU_CHIP_CUT(value) ( ( (value) >> 12 ) & 0x0f )

// --

S32
RTL8821CU_Probe( struct EthernetUnit *unit )
{
	U32 config;

	if ( ! RTL8821CU_RegisterIO( unit->unit_USB_Register->Res_Control, CMD_READ, RTL8821CU_SYS_CFG1, & config, sizeof( config ) ) )
	{
		return ( FALSE );
	}

	config = LE_SWAP32( config );

	if ( config == 0xffffffffU )
	{
		DebugPrintF( "RTL8821CU: invalid chip configuration value\n" );

		return ( FALSE );
	}

	unit->unit_Cut = RTL8821CU_CHIP_CUT( config );

	DebugPrintF( "RTL8821CU: Wi-Fi interface 2, SYS_CFG1 $%08lx, chip cut %lu\n",
		config, (U32) RTL8821CU_CHIP_CUT( config ) );

	return ( TRUE );
}

// --
