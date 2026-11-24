
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
RTL8811AU_StartRadio( struct EthernetUnit *unit, const struct RTL8811AU_Board *board, U32 max_packet )
{
	struct USB2_EPResource *control;

	control = unit->unit_Control;

	if ( ! RTL8811AU_InitMAC( control, unit->unit_StationAddress ) )
	{
		DebugPrintF( "RTL8811AU: MAC register initialization failed\n" );

		return ( FALSE );
	}

	if ( ! RTL8811AU_InitDMA( control, max_packet ) )
	{
		DebugPrintF( "RTL8811AU: DMA initialization failed\n" );

		return ( FALSE );
	}

	if ( ! RTL8811AU_InitBaseband( control, board ) )
	{
		DebugPrintF( "RTL8811AU: baseband initialization failed\n" );

		return ( FALSE );
	}

	if ( ! RTL8811AU_InitRF( control, board ) )
	{
		DebugPrintF( "RTL8811AU: RF initialization failed\n" );

		return ( FALSE );
	}

	return ( ( RTL8811AU_WriteReg( control, 0x060f, 1, 0 ) ) &&
		( RTL8811AU_UpdateReg( control, 0x0440, 4, 0x000fffff, 1 ) ) &&
		( RTL8811AU_WriteReg( control, 0x0608, 4, 0x0000280f ) ) &&
		( RTL8811AU_WriteReg( control, 0x06a0, 2, 0xffff ) ) &&
		( RTL8811AU_WriteReg( control, 0x06a2, 2, 0 ) ) &&
		( RTL8811AU_WriteReg( control, 0x06a4, 2, 0 ) ) &&
		( RTL8811AU_Update8( control, RTL8811AU_MAC_CTRL, 0x40, 0x80 ) ) &&
		( RTL8811AU_TuneRadio( unit, board, 1 ) ) );
}

// --
