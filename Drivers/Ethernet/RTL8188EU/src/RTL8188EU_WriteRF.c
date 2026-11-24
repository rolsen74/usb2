
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
RTL8188EU_WriteRF( struct USB2_EPResource *control, U16 address, U32 value )
{
	return ( RTL8188EU_Write32( control, 0x0840, ( (U32) address << 20 ) | ( value & 0x000fffff ) ) );
}

// --
