
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

void
Sana2_FreeList( struct List *list )
{
	struct Node *node;

	while( ( node = IExec->RemHead( list ) ) )
	{
		IExec->FreeVec( node );
	}
}

// --
