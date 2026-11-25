
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Commands.h"
#include "Radio.h"

// --

void
Cmd_Online( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	if ( ! unit->unit_Configured )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_STATE;
		ioreq->ios2_WireError = S2WERR_NOT_CONFIGURED;

		return;
	}

	if ( ! RTL8821CU_CanGoOnline( unit ) )
	{
		ReportUnitOffline( unit, ioreq );

		return;
	}

	unit->unit_ManualOffline = FALSE;

	RTL8821CU_SetOnline( unit, TRUE );
}

// --
