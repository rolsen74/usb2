
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

void
_fs_Release( void )
{
	ObtainSemaphore( &FS_PortGate );

	FS_Users--;

	ReleaseSemaphore( &FS_PortGate );
}

// --
