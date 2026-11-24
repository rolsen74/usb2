
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
RTL8188EU_Wait8( struct USB2_EPResource *control, U16 address, U8 mask, U8 value )
{
	U32 attempt;
	U8 status;

	for( attempt = 0; attempt < 20; attempt++ )
	{
		if ( ! RTL8188EU_RegisterIO( control, CMD_READ, address, & status, sizeof( status ) ) )
		{
			break;
		}

		if ( ( status & mask ) == value )
		{
			return ( TRUE );
		}

		Delay( 1 );
	}

	return ( FALSE );
}

// --
