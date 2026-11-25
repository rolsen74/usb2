
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
RTL8821CU_StartRadio( struct EthernetUnit *unit )
{
	struct USB2_EPResource *control;

	control = unit->unit_USB_Register->Res_Control;

	if ( ! RTL8821CU_InitDMA( control, unit->unit_Bulk_In->EndPointMax ) )
	{
		DebugPrintF( "RTL8821CU: packet DMA initialization failed\n" );

		return ( FALSE );
	}

	if ( ! RTL8821CU_InitPHY( unit ) )
	{
		DebugPrintF( "RTL8821CU: PHY initialization failed\n" );

		return ( FALSE );
	}

	// Use software CCMP and receive management frames before association.
	if ( ( ! RTL8821CU_RegisterIO( control, CMD_WRITE, 0x0610, unit->unit_StationAddress, RTL8821CU_ADDR_SIZE ) ) ||
		( ! RTL8821CU_WriteReg( control, 0x060f, 1, 0 ) ) ||
		( ! RTL8821CU_WriteReg( control, 0x0680, 2, 0 ) ) ||
		( ! RTL8821CU_WriteReg( control, 0x0608, 4, 0x0400220e ) ) ||
		( ! RTL8821CU_WriteReg( control, 0x0605, 2, 0x3030 ) ) ||
		( ! RTL8821CU_WriteReg( control, 0x06a0, 2, 0xffff ) ) ||
		( ! RTL8821CU_WriteReg( control, 0x06a2, 2, 0 ) ) ||
		( ! RTL8821CU_WriteReg( control, 0x06a4, 2, 0 ) ) ||
		( ! RTL8821CU_UpdateReg( control, 0x0440, 4, 0x000fffff, 1 ) ) ||
		( ! RTL8821CU_TuneRadio( unit, 1, FALSE ) ) )
	{
		DebugPrintF( "RTL8821CU: radio setup failed\n" );

		return ( FALSE );
	}

	if ( ! RTL8821CU_CalibrateRF( unit ) )
	{
		DebugPrintF( "RTL8821CU: RF calibration failed\n" );

		return ( FALSE );
	}

	if ( ! RTL8821CU_SetTxPower( unit ) )
	{
		return ( FALSE );
	}

	DebugPrintF( "RTL8821CU: listening for beacons on channel 1\n" );

	return ( TRUE );
}

// --
