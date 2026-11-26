
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

const char *
Read_Driver_Titles( USB2_ID id, U32 tag, STR buffer, U32 capacity, STR *allocated )
{
	U32 bytes;
	U32 actual;

	*allocated = NULL;

	bytes = USB2_Attr_Get( id, tag, buffer, capacity );

	if ( ! bytes )
	{
		return ( NULL );
	}

	if ( bytes <= capacity )
	{
		return ( buffer );
	}

	*allocated = AllocVecTags( bytes, TAG_END );

	if ( ! *allocated )
	{
		return ( NULL );
	}

	actual = USB2_Attr_Get( id, tag, *allocated, bytes );

	if ( ( actual ) && ( actual <= bytes ) )
	{
		return ( *allocated );
	}

	FreeVec( *allocated );

	*allocated = NULL;

	return ( NULL );
}

// --
