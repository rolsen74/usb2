
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Radio.h"
#include "Registers.h"

// --

S32
RTL8821CU_UpdateRF( struct USB2_EPResource *control, U8 address, U32 clear, U32 value )
{
	if ( ! address )
	{
		// RF register zero uses the serial path.
		return ( RTL8821CU_WriteReg( control, 0x0c90, 4, value & 0x000fffff ) );
	}

	return ( RTL8821CU_UpdateReg( control, 0x2800 + ( (U16) address << 2 ), 4, clear, value ) );
}

// --
