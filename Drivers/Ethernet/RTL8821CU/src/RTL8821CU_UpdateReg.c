
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Registers.h"

// --

S32
RTL8821CU_UpdateReg( struct USB2_EPResource *control, U16 address, U16 width, U32 mask, U32 value )
{
	U32 current;

	if ( ! RTL8821CU_ReadReg( control, address, width, & current ) )
	{
		return ( FALSE );
	}

	return ( RTL8821CU_WriteReg( control, address, width, ( current & ~mask ) | ( value & mask ) ) );
}

// --
