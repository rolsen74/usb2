
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
Printer_Abort( struct PrinterUnit *unit )
{
	struct USB2_IORequest *usbio;
	struct IOStdReq *request;
	struct IOStdReq *next;

	U32 index;

	for( index = 0; ( ! unit->unit_Buffer_Size ) && ( index < NR_of_TXs ); index++ )
	{
		usbio = unit->unit_Bulk_Tx_Active[index];

		if ( usbio )
		{
			request = usbio->io_UserData;

			if ( request->io_Flags & IOPARF_ABORT )
			{
				AbortIO( (PTR) usbio );
			}
		}
	}

	request = (PTR) GetHead( unit->unit_PAR_IOReq_WaitingList );

	while( request )
	{
		next = (PTR) GetSucc( (PTR) request );

		if ( request->io_Flags & IOPARF_ABORT )
		{
			Remove( (PTR) request );

			unit->unit_PAR_IOReq_WaitingCount--;
			request->io_Error = IOERR_ABORTED;

			ReplyMsg( (PTR) request );
		}

		request = next;
	}
}

// --
