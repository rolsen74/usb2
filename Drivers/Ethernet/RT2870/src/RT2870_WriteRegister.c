
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define RT2870_WRITE_16 0x02

// --

S32
RT2870_WriteWord( struct USB2_EPResource *control, U16 address, U16 value )
{
	return ( RT2870_Control( control, REQTYPE_Write, RT2870_WRITE_16, value, address, NULL, 0 ) );
}

// --

S32
RT2870_WriteRegister( struct USB2_EPResource *control, U16 address, U32 value )
{
	S32 error;

	error = RT2870_WriteWord( control, address, (U16) value );

	if ( ! error )
	{
		error = RT2870_WriteWord( control, address + 2, value >> 16 );
	}

	return ( error );
}

// --
