
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
Cmd_Flush( struct EthernetUnit *unit, struct IOSana2Req *ioreq UNUSED )
{
	Sana2_AbortPackets( & unit->unit_Pending, IOERR_ABORTED, 0 );
}

// --
