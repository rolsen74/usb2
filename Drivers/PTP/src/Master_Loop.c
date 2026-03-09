
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

void Master_Loop( struct intern *in )
{
U32 wait;
U32 mask;

	in->Running = TRUE;

	while( in->Running )
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
