
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "CompPro.h"

// --

U32 _main_Release( struct USBFDIFace *Self )
{
	Self->Data.RefCount--;

	return( Self->Data.RefCount );
}

// --
