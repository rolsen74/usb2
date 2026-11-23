
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void
Audio_Write_Account( struct AudioUnit *unit, U32 slot )
{
	struct USB2_IORequest *ioreq;
	struct AudioWrite *write;

	U32 framesize;
	U32 actual;
	U32 bytes;

	ioreq = unit->Stream->IORequests[slot];
	write = unit->WriteOwners[slot];
	bytes = unit->WriteBytes[slot];
	framesize = unit->Format.Channels * 2;
	actual = MIN( ioreq->io_Actual, bytes );
	write->Done += bytes;
	write->Actual += actual - actual % framesize;
	unit->WriteOwners[slot] = NULL;
}

// --
