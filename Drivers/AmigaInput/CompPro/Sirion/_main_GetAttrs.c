
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
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
