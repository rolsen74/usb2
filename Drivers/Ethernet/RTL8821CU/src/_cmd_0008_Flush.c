
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
Cmd_Flush( struct EthernetUnit *unit, S32 error, U32 wire )
{
	struct IOSana2Req *ioreq;
	struct IOSana2Req *next;

	ioreq = (PTR) GetHead( & unit->unit_Pending );

	while( ioreq )
	{
		next = (PTR) GetSucc( (PTR) ioreq );

		switch( ioreq->ios2_Req.io_Command )
		{
			case CMD_READ:
			case CMD_WRITE:
			case S2_BROADCAST:
			case S2_MULTICAST:
			case S2_READORPHAN:
			case S2_READMGMT:
			case S2_WRITEMGMT:
			{
				Remove( (PTR) ioreq );

				ioreq->ios2_Req.io_Error = error;
				ioreq->ios2_WireError = wire;

				ReplyMsg( (PTR) ioreq );

				break;
			}
		}

		ioreq = next;
	}
}

// --
