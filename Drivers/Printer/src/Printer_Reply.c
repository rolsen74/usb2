
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
Printer_Reply( struct IORequest *request )
{
	Disable();

	if ( request->io_Flags & IOPARF_ABORT )
	{
		request->io_Error = IOERR_ABORTED;

		if ( ( request->io_Command == PDCMD_QUERY ) && ( request->io_Message.mn_Length >= sizeof( struct IOExtPar ) ) )
		{
			((struct IOExtPar *)request)->IOPar.io_Actual = 0;
			((struct IOExtPar *)request)->io_Status = 0;
		}
	}

	ReplyMsg( (PTR)request );

	Enable();
}

// --
