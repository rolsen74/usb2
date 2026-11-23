
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
Cmd_Write_Direct( struct PrinterUnit *unit, struct IOStdReq *pario )
{
	USBERROR( "Printer : Cmd_Write_Direct : pario %p : length %ld", pario, pario->io_Length );

	if ( ! pario->io_Data )
	{
		pario->io_Actual = 0;
		pario->io_Error = ParErr_LineErr;

		ReplyMsg( (PTR) pario );

		return;
	}

	if ( ! pario->io_Length )
	{
		pario->io_Actual = 0;
		pario->io_Error = 0;

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
