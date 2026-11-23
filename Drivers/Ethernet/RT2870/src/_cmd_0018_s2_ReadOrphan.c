
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

S32
Cmd_ReadOrphan( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	return ( Cmd_Read( unit, ioreq ) );
}

// --
