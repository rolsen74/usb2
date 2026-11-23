
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
ReplySanaRequest( APTR adapter, struct IOSana2Req *request )
{
	struct devUnit *unit;
	struct SanaRequest *submission;

	unit = adapter;

	IExec->ObtainSemaphore( & unit->unit_DispatchLock );

	submission = FindSanaRequest( unit, request );

	if ( submission )
	{
		if ( submission->Cancelled )
		{
			request->ios2_Req.io_Error = IOERR_ABORTED;
			request->ios2_WireError = S2WERR_GENERIC_ERROR;
		}

		IExec->Remove( & submission->Node );
	}

	IExec->ReleaseSemaphore( & unit->unit_DispatchLock );

	if ( submission )
	{
		IExec->FreeVec( submission );

		IExec->ReplyMsg( (PTR) request );
	}
}

// --
