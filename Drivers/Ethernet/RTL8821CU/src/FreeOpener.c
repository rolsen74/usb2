
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

void
FreeOpener( struct BufferManagement *opener )
{
	Sana2_FreeList( & opener->bm_Multicast );

	Sana2_FreeList( & opener->bm_TrackedTypes );

	FreeVec( opener );
}

// --
