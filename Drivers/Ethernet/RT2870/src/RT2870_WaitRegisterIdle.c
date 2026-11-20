
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
RT2870_WaitRegisterIdle( struct USB2_EPResource *control, U16 address, U32 busy, U32 *value )
{
	U32 attempt;
	S32 error;

	for( attempt = 0; attempt < 10; attempt++ )
	{
		error = RT2870_ReadRegister( control, address, value );

		if ( error )
		{
			return ( error );
		}

		if ( ! ( *value & busy ) )
		{
			return ( 0 );
		}

		Delay( 1 );
	}

	return ( USB2Err_Host_Timeout );
}

// --
