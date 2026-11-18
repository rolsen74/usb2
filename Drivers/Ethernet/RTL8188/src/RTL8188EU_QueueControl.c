
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Firmware.h"
#include "Radio.h"

// --

void
RTL8188EU_QueueControl( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	AddTail( & unit->unit_Pending, (PTR) ioreq );

	Signal( unit->unit_TaskAdr, SIGBREAKF_CTRL_E );
}

// --
