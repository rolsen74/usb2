
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Sana2Commands.h"

// --

void
SignalSanaEvent( struct devUnit *unit, U32 events )
{
	Sana2_SignalEventWithReply( & unit->unit_EventsList, events, ReplySanaRequest, unit );
}

// --
