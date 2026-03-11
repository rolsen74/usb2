
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

void _fs_Handler__FSVector( struct FS_Struct *fs )
{
struct DosPacket *dp;
struct Message *msg;

	while( TRUE )
	{
		msg = GetMsg( & fs->fs_FSVector_Port->MP );

		if ( ! msg )
		{
			break;
		}

		dp = (PTR) msg->mn_Node.ln_Name;

		if ( dp )
		{
			DebugPrintF( "\n\nAction $%04lx\n", dp->dp_Action );

			// if ( ACTION_SHUTDOWN == dp->dp_Action )
			// {
			// 	Handle_Shutdown( fs, dp );
			// }
			// else
			{
				if( fs->fs_FSVector_Port->FSV.DOSEmulatePacket )
				{
					/* Pass all other packets to the dos packet emulator */
					fs->fs_FSVector_Port->FSV.DOSEmulatePacket( fs->fs_FSVector_Port, dp );
				}
				else
				{
					/* Emul vector was null before DOS 53.91 (OS4.1-U4 = 53.90) */
					dp->dp_Res1 = DOSFALSE;
					dp->dp_Res2 = ERROR_ACTION_NOT_KNOWN;
				}
			}
		}

		ReplyMsg( msg );
	}
}
