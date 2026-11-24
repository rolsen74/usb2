
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Firmware.h"

// --

S32
RTL8188EU_Write8( struct USB2_EPResource *control, U16 address, U8 value )
{
	return ( RTL8188EU_RegisterIO( control, CMD_WRITE, address, & value, sizeof( value ) ) );
}

// --
