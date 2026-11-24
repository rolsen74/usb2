
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

S32
RTL8811AU_ReadReg( struct USB2_EPResource *control, U16 address, U16 width, U32 *value )
{
	U32 data;

	data = 0;

	if ( ! RTL8811AU_RegisterIO( control, CMD_READ, address, & data, width ) )
	{
		return ( FALSE );
	}

	*value = LE_SWAP32( data );

	return ( TRUE );
}

// --
