
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define SIS163U_REGISTER_WRITE 0xfb

// --

S32
SiS163U_WriteRegister( struct USB2_EPResource *control, U16 address, U32 value )
{
	U8 data[4];

	data[0] = value;
	data[1] = value >> 8;
	data[2] = value >> 16;
	data[3] = value >> 24;

	return ( SiS163U_RegisterIO( control, CMD_WRITE, SIS163U_REGISTER_WRITE, address, data ) );
}

// --
