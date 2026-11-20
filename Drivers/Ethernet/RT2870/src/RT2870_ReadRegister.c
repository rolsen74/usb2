
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
RT2870_ReadRegister( struct USB2_EPResource *control, U16 address, U32 *value )
{
	U32 data;
	S32 error;

	error = RT2870_Control( control, REQTYPE_Read, RT2870_READ_REGION, 0, address, & data, sizeof( data ) );

	if ( ! error )
	{
		*value = LE_SWAP32( data );
	}

	return ( error );
}

// --
