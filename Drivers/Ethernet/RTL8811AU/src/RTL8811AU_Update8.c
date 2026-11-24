
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
RTL8811AU_Update8( struct USB2_EPResource *control, U16 address, U8 clear, U8 set )
{
	U32 value;

	if ( ! RTL8811AU_ReadReg( control, address, 1, &value ) )
	{
		return ( FALSE );
	}

	return ( RTL8811AU_WriteReg( control, address, 1, ( value & ~clear ) | set ) );
}

// --
