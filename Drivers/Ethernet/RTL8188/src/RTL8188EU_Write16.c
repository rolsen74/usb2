
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
RTL8188EU_Write16( struct USB2_EPResource *control, U16 address, U16 value )
{
	U8 data[2];

	data[0] = value;
	data[1] = value >> 8;

	return ( RTL8188EU_RegisterIO( control, CMD_WRITE, address, data, sizeof( data ) ) );
}

// --
