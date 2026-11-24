
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
Cmd_Write_Buffer( struct PrinterUnit *unit, struct IOStdReq *request )
{
	if ( ( ! request->io_Data ) || ( ! request->io_Length ) )
	{
		request->io_Error = ParErr_InvParam;

		Printer_Reply( (PTR)request );

		return;
	}

	AddTail( unit->unit_PAR_IOReq_WaitingList, (PTR)request );

	unit->unit_PAR_IOReq_WaitingCount++;
}

// --
