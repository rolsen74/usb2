
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

void
_fs_Handler__Commands( struct FS_Struct *fs )
{
	struct CommandMessage *msg;
	struct CameraInfoNode *camera;
	struct FS_ObjNode *node;

	STR name;

	ObtainSemaphore( & fs->fs_Semaphore );

	while(( msg = (PTR) GetMsg( FS_CmdMsgPort ) ))
	{
		camera = msg->cm_InfoNode;
		msg->cm_Error = 0;

		switch( msg->cm_Command )
		{
			case CMD_Camera_Added:
				name = ASPrintf( "Camera-%08lx%08lx", (U32) ( camera->cin_ID >> 32 ), (U32) camera->cin_ID );

				if ( ! name )
				{
					msg->cm_Error = ERROR_NO_FREE_STORE;
					break;
				}

				node = _fs_Node_CreateDir( fs, & msg->cm_Error, name );

				FreeVec( name );

				if ( node )
				{
					node->on_Parent_dir = fs->fs_RootNode;
					node->on_CameraRoot = node;
					node->on_Camera = camera;
					camera->cin_DirNode = node;

					AddTail( & fs->fs_RootNode->on_Content_List, & node->on_Node );
				}

				break;

			case CMD_Camera_Removed:
				node = camera->cin_DirNode;

				if ( node )
				{
					Remove( & node->on_Node );

					node->on_Camera = NULL;
					camera->cin_DirNode = NULL;

					if ( ! node->on_References )
					{
						_fs_Node_Delete( fs, node );
					}
				}

				break;

			default:
				msg->cm_Error = ERROR_ACTION_NOT_KNOWN;
				break;
		}

		ReplyMsg( (PTR) msg );
	}

	ReleaseSemaphore( & fs->fs_Semaphore );
}

// --
