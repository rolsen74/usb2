
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
SiS163U_WaitRegister( struct USB2_EPResource *control, U16 address, U32 mask )
{
	U32 poll;
	U32 value;
	S32 error;

	for( poll = 0; poll < 100; poll++ )
	{
		error = SiS163U_ReadRegister( control, address, & value );

		if ( error )
		{
			return ( error );
		}

		if ( ! ( value & mask ) )
		{
			return ( 0 );
		}
	}

	return ( USB2Err_Host_Timeout );
}

// --
