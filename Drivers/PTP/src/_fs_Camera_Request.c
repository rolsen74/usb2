
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

S32
_fs_Camera_Request( struct CameraInfoNode *camera, struct CommandMessage *request )
{
	struct MsgPort *reply;

	S32 queued;

	if ( ! camera )
	{
		return ( ERROR_DEVICE_NOT_MOUNTED );
	}

	reply = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! reply )
	{
		return ( ERROR_NO_FREE_STORE );
	}

	if ( request->cm_Command != CMD_Camera_Fence )
	{
		request->cm_Command = CMD_Camera_Transaction;
	}
	request->cm_Message.mn_ReplyPort = reply;
	request->cm_Message.mn_Length = sizeof( *request );

	ObtainSemaphore( & camera->cin_Gate );

	queued = ( camera->cin_Online ) &&
		( ( ! request->cm_Fenced ) || ( request->cm_Generation == camera->cin_StorageGeneration ) );

	if ( queued )
	{
		PutMsg( camera->cin_MsgPort, (PTR) request );
	}
	else
	{
		request->cm_Error = ERROR_DEVICE_NOT_MOUNTED;
	}

	ReleaseSemaphore( & camera->cin_Gate );

	if ( queued )
	{
		WaitPort( reply );

		GetMsg( reply );

		ObtainSemaphore( &camera->cin_Gate );

		if ( ( request->cm_Fenced ) && ( request->cm_Generation != camera->cin_StorageGeneration ) )
		{
			request->cm_Error = ERROR_DEVICE_NOT_MOUNTED;
		}

		ReleaseSemaphore( &camera->cin_Gate );
	}

	FreeSysObject( ASOT_PORT, reply );

	return ( request->cm_Error );
}

// --

S32
_fs_Camera_Request_Node( struct FS_ObjNode *node, struct CommandMessage *request )
{
	request->cm_Fenced = TRUE;
	request->cm_Generation = node->on_CameraRoot->on_StorageGeneration;

	return ( _fs_Camera_Request( node->on_CameraRoot->on_Camera, request ) );
}

// --

S32
_fs_Camera_Fence( struct FS_ObjNode *node )
{
	struct CommandMessage request;

	memset( &request, 0, sizeof( request ) );

	request.cm_Command = CMD_Camera_Fence;

	return ( _fs_Camera_Request_Node( node, &request ) );
}

// --
