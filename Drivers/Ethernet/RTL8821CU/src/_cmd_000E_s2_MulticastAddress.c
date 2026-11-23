
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Commands.h"

// --

void
Cmd_MulticastAddress( struct BufferManagement *opener, struct IOSana2Req *ioreq )
{
	Sana2_MulticastRange( & opener->bm_Multicast, ioreq );
}

// --
