
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "MSD.h"

// --

#ifdef MSD_DEBUG
SEC_CODE U32
MSD_TraceDue( struct MSDTraceState *trace, U32 result )
{
	if ( ( trace->Seen ) && ( trace->Result == result ) && ( ++trace->Repeats < 5 ) )
	{
		return ( FALSE );
	}

	trace->Seen = TRUE;
	trace->Result = result;
	trace->Repeats = 0;

	return ( TRUE );
}
#endif

// --
