
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Radio.h"

// --

S32
RTL8811AU_ReadRF( struct USB2_EPResource *control, U8 address, U32 *value )
{
	U32 mode;
	U16 readback;

	if ( ( ! RTL8811AU_ReadReg( control, 0x0c00, 4, &mode ) ) ||
		( ! RTL8811AU_Update8( control, 0x08b0, 0xff, address ) ) )
	{
		return ( FALSE );
	}

	readback = ( mode & 0x04 ) ? 0x0d04 : 0x0d08;

	if ( ! RTL8811AU_ReadReg( control, readback, 4, value ) )
	{
		return ( FALSE );
	}

	*value &= 0x000fffff;

	return ( TRUE );
}

// --
