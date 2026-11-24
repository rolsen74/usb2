
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
RTL8811AU_TuneRadio( struct EthernetUnit *unit, const struct RTL8811AU_Board *board, U8 channel )
{
	struct USB2_EPResource *control;

	U32 group;
	U32 power;
	U32 rf;

	control = unit->unit_Control;
	unit->unit_TxReady = FALSE;

	if ( ( ! RTL8811AU_WriteReg( control, RTL8811AU_TX_PAUSE, 1, 0xff ) ) ||
		( ! RTL8811AU_Update8( control, RTL8811AU_MAC_CTRL, 0x40, 0 ) ) ||
		( ! RTL8811AU_SetChannel( control, board, channel ) ) )
	{
		return ( FALSE );
	}

	unit->unit_Channel = channel;
	group = channel <= 2 ? 0 : ( channel <= 5 ? 1 : ( channel <= 8 ? 2 : 3 ) );
	power = board->CCKPower[group];

	if ( ! unit->unit_Tx || power > 0x3f )
	{
		DebugPrintF( "RTL8811AU: channel %lu is receive-only (CCK index $%02lx)\n", (U32) channel, power );

		return ( TRUE );
	}

	if ( ! RTL8811AU_ReadRF( control, 0x18, & rf ) )
	{
		return ( FALSE );
	}

	if ( ! RTL8811AU_WriteRF( control, 0x18, rf | 0x08000 ) )
	{
		return ( FALSE );
	}

	Delay( 5 );

	// TX descriptors select 1 Mbps CCK and suppress rate fallback.
	if ( ( ! RTL8811AU_WriteRF( control, 0x18, rf ) ) ||
		( ! RTL8811AU_UpdateReg( control, 0x082c, 4, 0x80000000U, 0 ) ) ||
		( ! RTL8811AU_WriteReg( control, 0x0c20, 4, power * 0x01010101U ) ) ||
		( ! RTL8811AU_Update8( control, RTL8811AU_MAC_CTRL, 0, 0x40 ) ) ||
		( ! RTL8811AU_WriteReg( control, RTL8811AU_TX_PAUSE, 1, 0 ) ) )
	{
		return ( FALSE );
	}

	unit->unit_TxReady = TRUE;

	DebugPrintF( "RTL8811AU: TX enabled on channel %lu, eFuse CCK power index $%02lx\n", (U32) channel, power );

	return ( TRUE );
}

// --
