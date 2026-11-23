
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
Process_RadioCommands( struct EthernetUnit *unit )
{
	struct IOSana2Req *ioreq;
	struct IOSana2Req *next;

	ObtainSemaphore( & unit->unit_CommandLock );

	ioreq = (PTR) GetHead( & unit->unit_Pending );

	while( ioreq )
	{
		next = (PTR) GetSucc( (PTR) ioreq );

		if ( ioreq->ios2_Req.io_Command == S2_SETOPTIONS )
		{
			Remove( (PTR) ioreq );

			Cmd_SetOptions( unit, ioreq );

			ReplyMsg( (PTR) ioreq );

			// A hardware error can signal and remove other pending requests.
			next = (PTR) GetHead( & unit->unit_Pending );
		}

		else if ( ioreq->ios2_Req.io_Command == S2_WRITEMGMT )
		{
			Remove( (PTR) ioreq );

			SiS163U_WriteMgmt( unit, ioreq );

			ReplyMsg( (PTR) ioreq );

			next = (PTR) GetHead( & unit->unit_Pending );
		}

		ioreq = next;
	}

	if ( ( unit->unit_ManualScan ) && ( ! unit->unit_RxActive ) && ( SiS163U_FindMgmtRead( unit ) ) )
	{
		SiS163U_SubmitRXCheck( unit );
	}

	ReleaseSemaphore( & unit->unit_CommandLock );
}

// --
