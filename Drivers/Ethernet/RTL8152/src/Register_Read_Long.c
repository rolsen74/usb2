
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

U32
myRead4( struct devUnit *unit, uint16 reg, uint16 index, U32 *result )
{
U32 temp;
int error;

	#ifdef DEBUG
	if (( index != URE_MCU_TYPE_USB ) && ( index != URE_MCU_TYPE_PLA ))
	{
		SHOWMSG( "Reg: %04lx, Index: %04lx", reg, index );
	}
	#endif

	error = myReadMem( unit, reg, index, (APTR) & temp, 4 );

	*result = error ? 0 : LE_SWAP32( temp );

	return(	error );
}

// --
