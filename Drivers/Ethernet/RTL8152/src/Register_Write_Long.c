
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
myWrite4( struct devUnit *unit, uint16 reg, uint16 index, U32 val )
{
U32 temp;
int error;

	#ifdef DEBUG
	if (( index != URE_MCU_TYPE_USB ) && ( index != URE_MCU_TYPE_PLA ))
	{
		SHOWMSG( "Reg: %04lx, Index: %04lx", reg, index );
	}
	#endif

	temp = LE_SWAP32( val );

	error = myWriteMem( unit, reg, index | URE_BYTE_EN_DWORD, (APTR) & temp, 4 );
	return(		error );
}

// --
