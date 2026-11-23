
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
RTL8821CU_WriteReg( struct USB2_EPResource *control, U16 address, U16 width, U32 value )
{
	U32 data;

	data = LE_SWAP32( value );

	return ( RTL8821CU_RegisterIO( control, CMD_WRITE, address, & data, width ) );
}

// --
