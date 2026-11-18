
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
	struct AbortIOMessage *abort;
	struct IORequest *request;
	struct Node *node;

	AbortReqOnList( & unit->unit_RxList, NULL, S2WERR_UNIT_OFFLINE );

	AbortReqOnList( & unit->unit_TxList, NULL, S2WERR_UNIT_OFFLINE );

	AbortReqOnList( & unit->unit_EventsList, NULL, S2WERR_UNIT_OFFLINE );

	AbortReqOnList( & unit->unit_SampleList, NULL, S2WERR_UNIT_OFFLINE );

	while(( node = IExec->RemHead( & unit->unit_MultiCastList ) ))
	{
		IExec->FreeVec( node );
	}

	if ( unit->unit_AbortIOMsgPort )
	{
		while(( abort = (PTR) IExec->GetMsg( unit->unit_AbortIOMsgPort ) ))
		{
			IExec->FreeVec( abort );
		}

		IExec->FreeSysObject( ASOT_PORT, unit->unit_AbortIOMsgPort );

		unit->unit_AbortIOMsgPort = NULL;
	}

	if ( unit->unit_BeginIOMsgPort )
	{
		while(( request = (PTR) IExec->GetMsg( unit->unit_BeginIOMsgPort ) ))
		{
			request->io_Error = IOERR_ABORTED;

			IExec->ReplyMsg( (PTR) request );
		}

		IExec->FreeSysObject( ASOT_PORT, unit->unit_BeginIOMsgPort );

		unit->unit_BeginIOMsgPort = NULL;
	}
}

// --
