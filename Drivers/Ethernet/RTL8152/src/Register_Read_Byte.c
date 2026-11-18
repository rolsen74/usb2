
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Controller.h"

// --

int
myRead1( struct devUnit *unit, uint16 reg, uint16 index, uint8 *result )
{
U32 temp;
U32 val;
uint8 shift;
int error;

	#ifdef DEBUG
	if (( index != URE_MCU_TYPE_USB ) && ( index != URE_MCU_TYPE_PLA ))
	{
		SHOWMSG( "Reg: %04lx, Index: %04lx", reg, index );
	}
	#endif

	shift	 = reg & 3;
	reg		&= ~3;

	error = myReadMem( unit, reg, index, (APTR) & temp, 4 );

	val = error ? 0 : LE_SWAP32( temp );
	val >>= ( shift * 8 );

	SHOWMSG( "Value: %02lx", val & 0xff );

	*result = val & 0xff;
	return(		error );
}

// --
