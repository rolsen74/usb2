
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
RTL8188EU_ResetMCU( struct USB2_EPResource *control )
{
	U16 address;

	address = RTL8188EU_SYS_FUNC_EN + 1;

	if ( ! RTL8188EU_Update8( control, address, RTL8188EU_CPU_ENABLE, 0 ) )
	{
		return ( FALSE );
	}

	Delay( 1 );

	if ( ! RTL8188EU_Update8( control, address, 0, RTL8188EU_CPU_ENABLE ) )
	{
		return ( FALSE );
	}

	Delay( 1 );

	return ( TRUE );
}

// --
