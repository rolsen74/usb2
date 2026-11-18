
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

void Cmd_Write_Direct( struct PrinterUnit *unit, struct IOExtPar *pario )
{
	USBERROR( "Printer : Cmd_Write_Direct : pario %p : length %ld", pario, pario->IOPar.io_Length );

	if ( ! pario->IOPar.io_Data )
	{
		pario->IOPar.io_Actual = 0;
		pario->IOPar.io_Error = ParErr_LineErr;
		pario->io_Status = 0;
		ReplyMsg( (PTR) pario );

		return;
	}

	if ( ! pario->IOPar.io_Length )
	{
		pario->IOPar.io_Actual = 0;
		pario->IOPar.io_Error = 0;
		pario->io_Status = 0;
		ReplyMsg( (PTR) pario );

		return;
	}

	// Add to unit IO list
	AddTail( unit->unit_PAR_IOReq_WaitingList, (PTR) pario );
	unit->unit_PAR_IOReq_WaitingCount++;

	// Start Sending if not running
	Direct_Schedule( unit );
}

// --
