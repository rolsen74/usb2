
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
RTL8188EU_SetChannel( struct USB2_EPResource *control, U32 channel )
{
	U32 rf;

	if ( ( channel < 1 ) || ( channel > 11 ) )
	{
		return ( FALSE );
	}

	rf = ( RTL8188EU_RF_CHANNEL_DEFAULT & ~RTL8188EU_RF_CHANNEL_MASK ) | RTL8188EU_RF_BW20 | channel;

	return ( RTL8188EU_Update8( control, 0x0603, 0, 0x04 ) &&
		RTL8188EU_Update32( control, 0x0800, 1, 0 ) &&
		RTL8188EU_Update32( control, 0x0900, 1, 0 ) &&
		RTL8188EU_WriteRF( control, RTL8188EU_RF_CHANNEL, rf ) );
}

// --
