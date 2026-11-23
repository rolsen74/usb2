
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Registers.h"

// --

S32
RTL8821CU_PollReg( struct USB2_EPResource *control, U16 address, U16 width, U32 mask, U32 wanted )
{
	U32 value;
	U32 attempt;

	for( attempt = 0; attempt < 24; attempt++ )
	{
		if ( ! RTL8821CU_ReadReg( control, address, width, & value ) )
		{
			return ( FALSE );
		}

		if ( ( value & mask ) == wanted )
		{
			return ( TRUE );
		}

		if ( attempt >= 3 )
		{
			Delay( 1 );
		}
	}

	DebugPrintF( "RTL8821CU: register $%04lx timed out (value $%08lx, mask $%08lx, expected $%08lx)\n",
		(U32) address, value, mask, wanted );

	return ( FALSE );
}

// --
