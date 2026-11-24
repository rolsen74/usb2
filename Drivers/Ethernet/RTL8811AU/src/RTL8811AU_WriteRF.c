
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
RTL8811AU_WriteRF( struct USB2_EPResource *control, U8 address, U32 value )
{
	return ( RTL8811AU_WriteReg( control, 0x0c90, 4, ( (U32) address << 20 ) | ( value & 0x000fffff ) ) );
}

// --
