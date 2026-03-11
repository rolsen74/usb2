
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

void Master_Loop( struct M_Struct *ms )
{
U32 wait;
U32 mask;

	MYERROR( "PTP : Master_Loop : Enter" );

	ms->Running = TRUE;

	while( ms->Running )
	{
		wait = SIGBREAKF_CTRL_C;

		mask = Wait( wait );

		if ( mask & SIGBREAKF_CTRL_C )
		{
			break;
		}
	}
}

// --
