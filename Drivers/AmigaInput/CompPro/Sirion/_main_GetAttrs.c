
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#include "CompPro.h"

// --

U32 _main_GetAttrsA( struct USBFDIFace *Self UNUSED, struct TagItem *taglist UNUSED )
{
	/* We don't support any tags - so we simply return */

	return( 0 );
}

// --

U32 _main_GetAttrs( struct USBFDIFace *Self UNUSED, ... )
{
	/* We don't support any tags - so we simply return */

	return( 0 );
}

// --
