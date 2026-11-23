
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
RTL8821CU_StopRX( struct EthernetUnit *unit )
{
	struct IORequest *ioreq;

	if ( ! unit->unit_RxPending )
	{
		return;
	}

	ioreq = (PTR) unit->unit_Bulk_In->IORequests[0];

	if ( ! CheckIO( ioreq ) )
	{
		AbortIO( ioreq );
	}

	WaitIO( ioreq );

	unit->unit_RxPending = FALSE;
}

// --
