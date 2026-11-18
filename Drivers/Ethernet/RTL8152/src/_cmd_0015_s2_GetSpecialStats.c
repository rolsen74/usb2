
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
s2_GetSpecialStats( struct devUnit *unit, struct IOSana2Req *ioreq )
{
	struct Sana2SpecialStatHeader *header;
	struct Sana2SpecialStatRecord *record;

	header = ioreq->ios2_StatData;
	ioreq->ios2_WireError = 0;

	if ( ! header )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_NULL_POINTER;
	}
	else
	{
		header->RecordCountSupplied = 0;

		if ( header->RecordCountMax )
		{
			record = (PTR) ( header + 1 );
			record->Type = S2SS_ETHERNET_BADMULTICAST;
			record->Count = unit->unit_MultiCastRejects;
			record->String = "Rejected multicast packets";
			header->RecordCountSupplied = 1;
		}
	}

	IExec->ReplyMsg( (PTR) ioreq );
}

// --
