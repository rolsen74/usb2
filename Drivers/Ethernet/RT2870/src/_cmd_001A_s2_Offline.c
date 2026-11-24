
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
Cmd_Offline( struct EthernetUnit *unit, struct IOSana2Req *ioreq UNUSED )
{
	if ( ( unit->unit_Online ) || ( unit->unit_Channel ) )
	{
		RT2870_DropLink( unit, unit->unit_Detached );
	}
	else
	{
		RT2870_SetOnline( unit, FALSE );
	}
}

// --
