
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "Sana2Commands.h"

// --

void
Sana2_GetSpecialStats( struct IOSana2Req *ioreq, const struct Sana2SpecialStatRecord *record )
{
	struct Sana2SpecialStatHeader *header;
	struct Sana2SpecialStatRecord *output;

	header = ioreq->ios2_StatData;

	if ( ! header )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_NULL_POINTER;

		return;
	}

	header->RecordCountSupplied = 0;

	if ( ( record ) && ( header->RecordCountMax ) )
	{
		output = (APTR) ( header + 1 );
		*output = *record;
		header->RecordCountSupplied = 1;
	}
}

// --
