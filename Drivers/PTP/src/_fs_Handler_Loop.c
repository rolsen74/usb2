
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

void _fs_Handler_Loop( struct FS_Struct *fs )
{
U32 mask;
U32 wait;

	fs->fs_Running = TRUE;

	while( fs->fs_Running == TRUE )
	{
		wait = 0;

		// FileSystem Vector
		wait |= fs->fs_FSVector_PortBit;

		// Public MsgPort
		wait |= fs->fs_CmdMsgPortBit;

		mask = Wait( wait );

		if ( mask & fs->fs_FSVector_PortBit )
		{
			_fs_Handler__FSVector( fs );
		}

		if ( mask & fs->fs_CmdMsgPortBit )
		{
			_fs_Handler__Commands( fs );
		}
	}
}

// --
