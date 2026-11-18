
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "Xbox360.h"

// --

U32
Xbox360_Obtain( struct Interface *Self )
{
	return ( Self->Data.RefCount++ );
}

// --
