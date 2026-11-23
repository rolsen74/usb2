
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <interfaces/utility.h>
#include "Sana2Commands.h"

// --

extern struct UtilityIFace *IUtility;

// --

int
Sana2_SetCopyHook( struct IOSana2Req *ioreq, struct Hook **slot, int replace )
{
	struct Sana2Hook *hook;

	hook = ioreq->ios2_Data;

	if ( ioreq->ios2_DataLength < sizeof( *hook ) )
	{
		ioreq->ios2_Req.io_Error = IOERR_BADLENGTH;
	}
	else if ( ( ! slot ) || ( ! hook ) || ( ! hook->s2h_Methods ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_NULL_POINTER;
	}
	else if ( ( ! replace ) && ( *slot ) )
	{
		ioreq->ios2_Req.io_Error = IOERR_UNITBUSY;
	}
	else if ( ( ! IUtility->TagInArray( S2_CopyToBuff, hook->s2h_Methods ) ) ||
		( ! IUtility->TagInArray( S2_CopyFromBuff, hook->s2h_Methods ) ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_FUNCTIONS_MISSING;
	}
	else
	{
		*slot = & hook->s2h_Hook;

		return ( TRUE );
	}

	return ( FALSE );
}

// --
