
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

int
AbortAll( struct devUnit *unit, struct BufferManagement *back_ptr, U32 wire_error )
{
S32 num_aborted;
int aborted;

	aborted = FALSE;

	num_aborted = AbortReqOnListBackPtr( unit, & unit->unit_RxList, back_ptr, wire_error );

	if ( num_aborted > 0 )
	{
		aborted = TRUE;
	}

	num_aborted = AbortReqOnListBackPtr( unit, & unit->unit_TxList, back_ptr, wire_error );

	if ( num_aborted > 0 )
	{
		aborted = TRUE;
	}

	return(	aborted );
}

// --
