
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

		if ( ( dp ) && ( dp->dp_Link == msg ) )
		{
			msg->mn_ReplyPort = dp->dp_Port;

			if ( ! fs->fs_Running )
			{
				dp->dp_Res1 = DOSFALSE;
				dp->dp_Res2 = ERROR_DEVICE_NOT_MOUNTED;
			}
			else if ( dp->dp_Action == ACTION_SHUTDOWN )
			{
				dp->dp_Res2 = _fs_Handler_Shutdown( fs );
				dp->dp_Res1 = dp->dp_Res2 ? DOSFALSE : DOSTRUE;

				if ( ! dp->dp_Res2 )
				{
					fs->fs_ShutdownMsg = msg;
					continue;
				}
			}
			else if ( fs->fs_FSVector_Port->FSV.DOSEmulatePacket )
			{
				fs->fs_FSVector_Port->FSV.DOSEmulatePacket( fs->fs_FSVector_Port, dp );
			}
			else
			{
				dp->dp_Res1 = DOSFALSE;
				dp->dp_Res2 = ERROR_ACTION_NOT_KNOWN;
			}
		}

		ReplyMsg( msg );
	}
}
