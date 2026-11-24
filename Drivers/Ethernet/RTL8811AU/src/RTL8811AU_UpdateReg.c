
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
RTL8811AU_UpdateReg( struct USB2_EPResource *control, U16 address, U16 size, U32 clear, U32 set )
{
	U32 value;

	if ( ! RTL8811AU_ReadReg( control, address, size, &value ) )
	{
		return ( FALSE );
	}

	return ( RTL8811AU_WriteReg( control, address, size, ( value & ~clear ) | set ) );
}

// --
