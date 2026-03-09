
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

void _fs_Handler__Commands( struct PTP_FSStruct *fs )
{
struct CommandMessage *msg;
S32 reply;

//	ObtainSemaphore( &fs->fs_Semaphore );

	while( TRUE )
	{
		msg = (PTR) GetMsg( fs->fs_CmdMsgPort );

		if ( ! msg )
		{
			break;
		}

		switch( msg->cm_Command )
		{
			// case PTPCMD_Camera_Added:
			// {
			// 	Handle_Camera_Add( fs, msg );
			// 	break;
			// }

			// case PTPCMD_Camera_Removed:
			// {
			// 	Handle_Camera_Remove( fs, msg );
			// 	break;
			// }

//--		case PTPCMD_Storage_Added:
//--		{
//--			Handle_Storage_Add( fs, msg );
//--			break;
//--		}

//--		case PTPCMD_Storage_Removed:
//--		{
//--			Handle_Storage_Remove( fs, msg );
//--			break;
//--		}

			default:
			{
				MYERROR( "PTP-FS : Unknown Command (%ld)", msg->cm_Command );
				reply = TRUE;
				break;
			}
		}

		if ( reply )
		{
			ReplyMsg( (PTR) msg );
		}
	}

//	ReleaseSemaphore( &fs->fs_Semaphore );
}
