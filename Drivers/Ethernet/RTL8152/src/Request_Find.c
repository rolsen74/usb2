
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

struct SanaRequest *
FindSanaRequest( struct devUnit *unit, const struct IOSana2Req *request )
{
	struct SanaRequest *submission;

	// Caller holds the dispatch lock.
	for ( submission = (PTR) IExec->GetHead( & unit->unit_Requests ); submission;
		submission = (PTR) IExec->GetSucc( & submission->Node ) )
	{
		if ( submission->Request == request )
		{
			break;
		}
	}

	return ( submission );
}

// --
