
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Commands.h"
#include "Radio.h"

// --

void
Process_RadioCommands( struct EthernetUnit *unit )
{
	struct IOSana2Req *ioreq;

	ObtainSemaphore( & unit->unit_CommandLock );

	for( ;; )
	{
		ioreq = (PTR) GetHead( & unit->unit_Pending );

		while( ioreq )
		{
			if ( ( ioreq->ios2_Req.io_Command == S2_SETOPTIONS ) ||
				( ioreq->ios2_Req.io_Command == S2_SETKEY ) ||
				( ioreq->ios2_Req.io_Command == S2_WRITEMGMT ) ||
				( ioreq->ios2_Req.io_Command == CMD_WRITE ) ||
				( ioreq->ios2_Req.io_Command == S2_MULTICAST ) ||
				( ioreq->ios2_Req.io_Command == S2_BROADCAST ) )
			{
				break;
			}

			ioreq = (PTR) GetSucc( (PTR) ioreq );
		}

		if ( ! ioreq )
		{
			break;
		}

		Remove( (PTR) ioreq );

		// The USB receive port belongs to this task, including during WaitIO.
		if ( ioreq->ios2_Req.io_Command == S2_SETOPTIONS )
		{
			Cmd_SetOptions( unit, ioreq );
		}
		else if ( ioreq->ios2_Req.io_Command == S2_SETKEY )
		{
			Cmd_SetKey( unit, ioreq );
		}
		else
		{
			RTL8188EU_Transmit( unit, ioreq );
		}

		ReplyMsg( (PTR) ioreq );
	}

	ReleaseSemaphore( & unit->unit_CommandLock );
}

// --
