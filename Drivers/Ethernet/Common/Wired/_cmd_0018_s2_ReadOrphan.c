
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
s2_ReadOrphan( struct devUnit *unit, struct IOSana2Req *ioreq )
{
	// The receive queue identifies orphan reads by their command.
	Cmd_Read( unit, ioreq );
}

// --
