
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
_usb_Loop( struct USB_Struct *us )
{
	struct CommandMessage notice;
	struct CommandMessage *request;
	struct CameraInfoNode *camera;
	struct USB2_IORequest *event;

	U32 signals;
	U32 wait;
	U32 eventbit;

	if ( ! FS_CmdMsgPort )
	{
		return;
	}

	memset( & notice, 0, sizeof( notice ) );

	camera = & us->us_CameraNode;
	camera->cin_Online = TRUE;
	notice.cm_Command = CMD_Camera_Added;
	notice.cm_InfoNode = camera;
	notice.cm_Message.mn_ReplyPort = us->us_CmdMsgPort;
	notice.cm_Message.mn_Length = sizeof( notice );

	PutMsg( FS_CmdMsgPort, (PTR) & notice );

	// The filesystem can queue reads as soon as the camera is published.
	wait = us->us_Register->Stack_MsgPortBit | ( 1UL << us->us_CmdMsgPort->mp_SigBit ) | SIGBREAKF_CTRL_C;
	event = NULL;
	eventbit = 0;

	if ( us->us_Res_Events )
	{
		eventbit = us->us_Res_Events->MsgPortBit;
		wait |= eventbit;
		event = us->us_Res_Events->IORequests[0];
		event->io_Command = CMD_READ;

		SendIO( (PTR) event );
	}

	while( ( us->us_Running ) && ( ! us->us_Detached ) )
	{
		signals = Wait( wait );

		if ( signals & us->us_Register->Stack_MsgPortBit )
		{
			_usb_Loop_Stack( us );
		}

		if ( ( us->us_Detached ) || ( signals & SIGBREAKF_CTRL_C ) )
		{
			break;
		}

		if ( ( signals & eventbit ) && ( GetMsg( us->us_Res_Events->MsgPort ) ) )
		{
			if ( _usb_Read_Events( us, event ) )
			{
				SendIO( (PTR) event );
			}
			else
			{
				wait &= ~eventbit;
				eventbit = 0;
				event = NULL;
			}
		}

		while(( request = (PTR) GetMsg( us->us_CmdMsgPort ) ))
		{
			if ( request == & notice )
			{
				if ( notice.cm_Error )
				{
					us->us_Running = FALSE;
				}

				continue;
			}

			if ( us->us_Running )
			{
				_usb_Transaction( us, request );
			}
			else
			{
				request->cm_Error = ERROR_DEVICE_NOT_MOUNTED;
			}

			ReplyMsg( (PTR) request );
		}
	}

	if ( event )
	{
		if ( ! CheckIO( (PTR) event ) )
		{
			AbortIO( (PTR) event );
		}

		WaitIO( (PTR) event );
	}

	// Stop new requests before replying to any caller waiting under the filesystem lock.
	ObtainSemaphore( & camera->cin_Gate );

	camera->cin_Online = FALSE;

	ReleaseSemaphore( & camera->cin_Gate );

	while(( request = (PTR) GetMsg( us->us_CmdMsgPort ) ))
	{
		if ( request == & notice )
		{
			continue;
		}

		request->cm_Error = ERROR_DEVICE_NOT_MOUNTED;

		ReplyMsg( (PTR) request );
	}

	// Use a separate notice so a racing attach acknowledgement stays valid.
	{
		struct CommandMessage remove;

		memset( & remove, 0, sizeof( remove ) );

		remove.cm_Command = CMD_Camera_Removed;
		remove.cm_InfoNode = camera;
		remove.cm_Message.mn_ReplyPort = us->us_CmdMsgPort;
		remove.cm_Message.mn_Length = sizeof( remove );

		PutMsg( FS_CmdMsgPort, (PTR) & remove );

		do
		{
			WaitPort( us->us_CmdMsgPort );

			request = (PTR) GetMsg( us->us_CmdMsgPort );
		}
		while( request != & remove );
	}
}

// --
