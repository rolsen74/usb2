
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Firmware.h"
#include "Radio.h"

// --

S32
RTL8188EU_TuneRadio( struct EthernetUnit *unit, U32 channel )
{
	struct USB2_EPResource *control;

	U32 group;
	U32 power;
	U32 rf;

	control = unit->unit_USB_Register->Res_Control;
	unit->unit_TxReady = FALSE;

	if ( ( ! RTL8188EU_Write8( control, 0x0522, 0xff ) ) ||
		( ! RTL8188EU_Update8( control, 0x0100, 0x40, 0 ) ) ||
		( ! RTL8188EU_SetChannel( control, channel ) ) )
	{
		return ( FALSE );
	}

	unit->unit_Channel = channel;
	group = channel <= 2 ? 0 : ( channel <= 5 ? 1 : ( channel <= 8 ? 2 : 3 ) );
	power = unit->unit_CCKPower[group];

	if ( power > 0x3f )
	{
		// A board without a valid eFuse power index stays receive-only.
		return ( TRUE );
	}

	rf = ( RTL8188EU_RF_CHANNEL_DEFAULT & ~RTL8188EU_RF_CHANNEL_MASK ) | RTL8188EU_RF_BW20 | channel;

	if ( ! RTL8188EU_WriteRF( control, RTL8188EU_RF_CHANNEL, rf | 0x08000 ) )
	{
		return ( FALSE );
	}

	Delay( 5 );

	// Only the calibrated 1 Mbps CCK rate is used for transmission.
	if ( ( ! RTL8188EU_WriteRF( control, RTL8188EU_RF_CHANNEL, rf ) ) ||
		( ! RTL8188EU_Update32( control, 0x0e08, 0x0000ff00, power << 8 ) ) ||
		( ! RTL8188EU_Write8( control, 0x0423, 0xff ) ) ||
		( ! RTL8188EU_Update8( control, 0x0100, 0, 0x40 ) ) ||
		( ! RTL8188EU_Write8( control, 0x0522, 0 ) ) )
	{
		return ( FALSE );
	}

	unit->unit_TxReady = TRUE;

	return ( TRUE );
}

// --
