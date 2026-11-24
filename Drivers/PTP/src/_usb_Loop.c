
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

static void
StartEvents( struct USB_Struct *us )
{
	struct CameraInfoNode *camera;

	camera = &us->us_CameraNode;

	if ( us->us_Res_Events )
	{
		us->us_EventUsed = 0;
		us->us_EventErrors = 0;
		us->us_EventIO = us->us_Res_Events->IORequests[0];
		us->us_EventIO->io_Command = CMD_READ;

		ObtainSemaphore( &camera->cin_Gate );

		camera->cin_EventsReliable = TRUE;

		ReleaseSemaphore( &camera->cin_Gate );

		SendIO( (PTR)us->us_EventIO );
	}
}

// --

static void
StopEvents( struct USB_Struct *us )
{
	if ( us->us_EventIO )
	{
		if ( ! CheckIO( (PTR)us->us_EventIO ) )
		{
			AbortIO( (PTR)us->us_EventIO );
		}

		WaitIO( (PTR)us->us_EventIO );

		us->us_EventIO = NULL;
	}
}

// --

void
_usb_Service_Events( struct USB_Struct *us )
{
	struct USB2_IORequest *event;
	struct USB2IFace *usb;

	_usb_Loop_Stack( us );

	event = us->us_EventIO;

	if ( ( ! event ) || ( us->us_Detached ) || ( ! GetMsg( us->us_Res_Events->MsgPort ) ) )
	{
		return;
	}

	if ( _usb_Read_Events( us, event ) )
	{
		us->us_EventErrors = 0;

		SendIO( (PTR)event );

		return;
	}

	us->us_EventUsed = 0;
	us->us_EventErrors++;
	usb = us->us_StartMsg->IUSB2;

	if ( ( ! us->us_Detached ) && ( us->us_EventErrors < 3 ) &&
		( ( event->io_Error != USB2Err_Host_Stall ) || ( usb->USB2_EPRes_Destall( us->us_Res_Events ) ) ) )
	{
		_usb_Invalidate_Storage( us, FALSE );

		SendIO( (PTR)event );
	}
	else
	{
		us->us_EventIO = NULL;

		_usb_Invalidate_Storage( us, TRUE );
	}
}

// --

S32
_usb_Check_Fence( struct USB_Struct *us, struct CommandMessage *request )
{
	struct CameraInfoNode *camera;

	S32 valid;

	if ( us->us_Detached )
	{
		return ( FALSE );
	}

	if ( ! request->cm_Fenced )
	{
		return ( TRUE );
	}

	camera = &us->us_CameraNode;

	ObtainSemaphore( &camera->cin_Gate );

	valid = request->cm_Generation == camera->cin_StorageGeneration;

	ReleaseSemaphore( &camera->cin_Gate );

	return ( valid );
}

// --

void
_usb_Loop( struct USB_Struct *us )
{
	struct CommandMessage notice;
	struct CommandMessage remove;
	struct CommandMessage *request;
	struct CameraInfoNode *camera;

	U32 signals;
	U32 wait;

	if ( ! FS_CmdMsgPort )
	{
		return;
	}

	memset( &notice, 0, sizeof( notice ) );

	camera = &us->us_CameraNode;
	camera->cin_Online = TRUE;
	notice.cm_Command = CMD_Camera_Added;
	notice.cm_InfoNode = camera;
	notice.cm_Message.mn_ReplyPort = us->us_CmdMsgPort;
	notice.cm_Message.mn_Length = sizeof( notice );

	PutMsg( FS_CmdMsgPort, (PTR)&notice );

	wait = us->us_Register->Stack_MsgPortBit | ( 1UL << us->us_CmdMsgPort->mp_SigBit ) | SIGBREAKF_CTRL_C;

	if ( us->us_Res_Events )
	{
		wait |= us->us_Res_Events->MsgPortBit;
	}

	StartEvents( us );

	while ( ( us->us_Running ) && ( ! us->us_Detached ) )
	{
		signals = Wait( wait );

		_usb_Service_Events( us );

		if ( ( us->us_Detached ) || ( signals & SIGBREAKF_CTRL_C ) )
		{
			break;
		}

		while ( ( us->us_Running ) && ( ! us->us_Detached ) &&
			( request = (PTR)GetMsg( us->us_CmdMsgPort ) ) )
		{
			if ( request == &notice )
			{
				us->us_Published = ! notice.cm_Error;

				if ( ! us->us_Published )
				{
					us->us_Running = FALSE;
				}

				continue;
			}

			_usb_Service_Events( us );

			if ( ! _usb_Check_Fence( us, request ) )
			{
				request->cm_Error = ERROR_DEVICE_NOT_MOUNTED;
			}
			else if ( request->cm_Command == CMD_Camera_Fence )
			{
				request->cm_Error = 0;
			}
			else
			{
				_usb_Transaction( us, request );
			}

			_usb_Service_Events( us );

			if ( ( us->us_TransportFailed ) || ( ! _usb_Check_Fence( us, request ) ) )
			{
				if ( ! request->cm_Error )
				{
					request->cm_Error = ERROR_DEVICE_NOT_MOUNTED;
				}
			}

			if ( ( us->us_TransportFailed ) && ( ! us->us_Detached ) )
			{
				_usb_Invalidate_Storage( us, TRUE );
			}

			ReplyMsg( (PTR)request );

			if ( ( us->us_TransportFailed ) && ( ! us->us_Detached ) )
			{
				StopEvents( us );

				us->us_Running = _usb_Recover( us );

				if ( us->us_Running )
				{
					StartEvents( us );
				}
			}
		}
	}

	ObtainSemaphore( &camera->cin_Gate );

	camera->cin_Online = FALSE;

	ReleaseSemaphore( &camera->cin_Gate );

	StopEvents( us );

	while ( ( request = (PTR)GetMsg( us->us_CmdMsgPort ) ) )
	{
		if ( request == &notice )
		{
			us->us_Published = ! notice.cm_Error;
		}
		else
		{
			request->cm_Error = ERROR_DEVICE_NOT_MOUNTED;

			ReplyMsg( (PTR)request );
		}
	}

	memset( &remove, 0, sizeof( remove ) );

	remove.cm_Command = CMD_Camera_Removed;
	remove.cm_InfoNode = camera;
	remove.cm_Message.mn_ReplyPort = us->us_CmdMsgPort;
	remove.cm_Message.mn_Length = sizeof( remove );

	PutMsg( FS_CmdMsgPort, (PTR)&remove );

	do
	{
		WaitPort( us->us_CmdMsgPort );

		request = (PTR)GetMsg( us->us_CmdMsgPort );

		if ( request == &notice )
		{
			us->us_Published = ! notice.cm_Error;
		}
	}
	while ( request != &remove );
}

// --
