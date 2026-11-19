/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Audio_Read_Reply( struct AudioUnit *unit, S8 error )
{
struct IOStdReq *ioreq;

	ioreq = unit->ReadActive;

	if ( ! ioreq )
	{
		return;
	}

	unit->ReadActive = NULL;
	ioreq->io_Error = error;
	ioreq->io_Actual = unit->DataUsed - unit->WriteLostBytes;
	unit->DataUsed = 0;
	unit->WriteLostBytes = 0;

	ReplyMsg( (PTR) ioreq );
}

// --
