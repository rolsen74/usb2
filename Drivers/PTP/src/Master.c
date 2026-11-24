
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

struct MsgPort *FS_CmdMsgPort = NULL;
struct SignalSemaphore FS_PortGate;
U32 FS_Users;
S32 FS_Closing;

// --

void
Master_Entry( void )
{
	struct TaskStartMsg *msg;
	struct DevProc *device;

	PTR window;

	msg = (PTR)FindTask( NULL )->tc_UserData;

	msg->Result = MountDeviceTags( "PTP:", MDT_Handler,
		MD_Handler, MDT_FileSystem,
		MD_Entry, _fs_Handler,
		MD_Activate, TRUE,
		TAG_END );

	if ( ( ! msg->Result ) && ( msg->Restart ) )
	{
		// A soft dismount can leave a DOS device entry ready to restart.
		window = SetProcWindow( (PTR)-1 );

		device = GetDeviceProc( "PTP:", NULL );

		if ( device )
		{
			FreeDeviceProc( device );
		}

		SetProcWindow( window );
	}

	ObtainSemaphore( &FS_PortGate );

	msg->Result = ( FS_CmdMsgPort != NULL ) && ( ! FS_Closing );

	ReleaseSemaphore( &FS_PortGate );
}

// --

void
Master_Exit( int32 result UNUSED, int32 data )
{
	struct TaskStartMsg *msg;

	msg = (PTR)data;

	// Keep the parent from rolling back the library until this callback exits.
	Forbid();

	PutMsg( msg->Message.mn_ReplyPort, &msg->Message );

}

// --
