
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

void _fs_Handler__Commands( struct FS_Struct *fs )
{
struct CommandMessage *msg;
struct CameraInfoNode *cin;
S32 reply;

	ObtainSemaphore( & fs->fs_Semaphore );

	while( TRUE )
	{
		msg = (PTR) GetMsg( FS_CmdMsgPort );

		if ( ! msg )
		{
			break;
		}

		switch( msg->cm_Command )
		{
			case CMD_Camera_Added:
			{
				MYERROR( "PTP-FS : CMD_Camera_Added" );

				cin = (PTR) GetHead( & fs->fs_CameraList );

				while( cin )
				{
					if ( msg->cm_InfoNode == cin )
					{
						break;
					}
					else
					{
						cin = (PTR) GetSucc( (PTR) cin );
					}
				}

				if ( ! cin )
				{
					cin = msg->cm_InfoNode;

					AddTail( & fs->fs_CameraList, (PTR) cin );

					cin->cin_DirNode = _fs_Node_CreateDir( fs, NULL, cin->cin_Name );

					AddTail( & fs->fs_RootNode->on_Content_List, (PTR) cin->cin_DirNode );
				}

				reply = FALSE;
				break;
			}

			case CMD_Camera_Removed:
			{
				MYERROR( "PTP-FS : CMD_Camera_Removed" );

				cin = (PTR) GetHead( & fs->fs_CameraList );

				while( cin )
				{
					if ( msg->cm_InfoNode == cin )
					{
						break;
					}
					else
					{
						cin = (PTR) GetSucc( (PTR) cin );
					}
				}

				if ( cin )
				{
					Remove( (PTR) cin );
//					FreeVec( msg );
				}

				FreeVec( msg );
				reply = FALSE;
				break;
			}

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

	ReleaseSemaphore( & fs->fs_Semaphore );
}
