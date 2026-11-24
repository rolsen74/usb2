
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
RTL8811AU_PollReg( struct USB2_EPResource *control, U16 address, U16 size, U32 mask, U32 wanted, U32 polls, U32 *value )
{
	U32 count;
	U32 status;

	status = 0;

	for( count = 0; count < polls; count++ )
	{
		if ( ! RTL8811AU_ReadReg( control, address, size, &status ) )
		{
			return ( FALSE );
		}

		if ( ( status & mask ) == wanted )
		{
			if ( value )
			{
				*value = status;
			}

			return ( TRUE );
		}

		if ( count + 1 < polls )
		{
			Delay( 1 );
		}
	}

	DebugPrintF( "RTL8811AU: register $%04lx timed out at $%08lx\n", (U32) address, status );

	return ( FALSE );
}

// --
