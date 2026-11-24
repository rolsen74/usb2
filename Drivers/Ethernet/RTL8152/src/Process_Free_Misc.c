
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
unit_Process_FreeMisc( struct devUnit *unit )
{
	struct IORequest *request;

	RTL8152_CloseLinkTimer( unit );

	AbortReqOnList( unit, & unit->unit_RxList, NULL, S2WERR_UNIT_OFFLINE );

	AbortReqOnList( unit, & unit->unit_TxList, NULL, S2WERR_UNIT_OFFLINE );

	AbortReqOnList( unit, & unit->unit_EventsList, NULL, S2WERR_UNIT_OFFLINE );

	AbortReqOnList( unit, & unit->unit_SampleList, NULL, S2WERR_UNIT_OFFLINE );

	Sana2_FreeList( & unit->unit_MulticastRanges );

	Sana2_FreeList( & unit->unit_TrackedTypes );

	Sana2_FreeList( & unit->unit_MultiCastList );

	if ( unit->unit_BeginIOMsgPort )
	{
		while(( request = (PTR) IExec->GetMsg( unit->unit_BeginIOMsgPort ) ))
		{
			request->io_Error = IOERR_ABORTED;

			ReplySanaRequest( unit, (PTR) request );
		}

		IExec->FreeSysObject( ASOT_PORT, unit->unit_BeginIOMsgPort );

		unit->unit_BeginIOMsgPort = NULL;
	}
}

// --
