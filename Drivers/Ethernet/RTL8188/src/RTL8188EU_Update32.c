
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
RTL8188EU_Update32( struct USB2_EPResource *control, U16 address, U32 clear, U32 set )
{
	U8 data[4];
	U32 value;

	if ( ! RTL8188EU_RegisterIO( control, CMD_READ, address, data, sizeof( data ) ) )
	{
		return ( FALSE );
	}

	value = data[0] | ( (U32) data[1] << 8 ) | ( (U32) data[2] << 16 ) | ( (U32) data[3] << 24 );

	return ( RTL8188EU_Write32( control, address, ( value & ~clear ) | set ) );
}

// --
