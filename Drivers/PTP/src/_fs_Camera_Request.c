
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

int32
_fs_Camera_Request( struct CameraInfoNode *camera, struct CommandMessage *request )
{
	struct MsgPort *reply;

	int32 queued;

	if ( ! camera )
	{
		return ( ERROR_DEVICE_NOT_MOUNTED );
	}

	reply = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! reply )
	{
		return ( ERROR_NO_FREE_STORE );
	}

	request->cm_Command = CMD_Camera_Transaction;
	request->cm_Message.mn_ReplyPort = reply;
	request->cm_Message.mn_Length = sizeof( *request );

	ObtainSemaphore( & camera->cin_Gate );

	queued = camera->cin_Online;

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
	}

	FreeSysObject( ASOT_PORT, reply );

	return ( request->cm_Error );
}

// --
