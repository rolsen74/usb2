
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
RTL8188EU_Update8( struct USB2_EPResource *control, U16 address, U8 clear, U8 set )
{
	U8 value;

	if ( ! RTL8188EU_RegisterIO( control, CMD_READ, address, & value, sizeof( value ) ) )
	{
		return ( FALSE );
	}

	return ( RTL8188EU_Write8( control, address, ( value & ~clear ) | set ) );
}

// --
