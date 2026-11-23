
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <interfaces/exec.h>
#include "Sana2Commands.h"

// --

extern struct ExecIFace *IExec;

// --

int
Sana2_QueueEvent( struct List *list, struct IOSana2Req *ioreq, uint32 current, uint32 supported )
{
	uint32 events;

	events = ioreq->ios2_WireError;

	if ( ( ! events ) || ( events & ~supported ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_NOT_SUPPORTED;
		ioreq->ios2_WireError = S2WERR_BAD_EVENT;

		return ( FALSE );
	}

	if ( events & current )
	{
		ioreq->ios2_WireError = current;

		return ( FALSE );
	}

	IExec->AddTail( list, (APTR) ioreq );

	return ( TRUE );
}

// --
