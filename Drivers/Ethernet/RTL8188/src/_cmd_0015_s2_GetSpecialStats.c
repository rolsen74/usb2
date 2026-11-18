
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
Cmd_GetSpecialStats( struct EthernetUnit *unit UNUSED, struct IOSana2Req *ioreq )
{
	struct Sana2SpecialStatHeader *stats;

	stats = ioreq->ios2_StatData;

	if ( ! stats )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_NULL_POINTER;

		return;
	}

	stats->RecordCountSupplied = 0;
}

// --
