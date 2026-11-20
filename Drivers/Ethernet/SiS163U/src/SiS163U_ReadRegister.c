
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define SIS163U_REGISTER_READ 0xfc

// --

S32
SiS163U_ReadRegister( struct USB2_EPResource *control, U16 address, U32 *value )
{
	U8 data[4];
	S32 error;

	error = SiS163U_RegisterIO( control, CMD_READ, SIS163U_REGISTER_READ, address, data );

	if ( ! error )
	{
		*value = (U32) data[0] | ( (U32) data[1] << 8 ) | ( (U32) data[2] << 16 ) | ( (U32) data[3] << 24 );
	}

	return ( error );
}

// --
