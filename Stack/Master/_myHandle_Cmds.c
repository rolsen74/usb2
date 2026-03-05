
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "_Cmd_Start_Process.c"
#include "_Cmd_Stop_Process.c"
#include "_Cmd_Fkt_Detach.c"

// --

SEC_CODE static void myHandle_Cmds( struct USBBase *usbbase, struct intern *in )
{
struct CmdHeader *msg;
S32 reply;

//	usbbase->usb_IExec->DebugPrintF( "myHandle_Cmds\n" );

	while( TRUE )
	{
		msg = MSGPORT_GETMSG( & in->Cmd_MsgPort );

		if ( ! msg )
		{
			break;
		}

		switch( msg->cm_Command )
		{
			case CMD_START_PROCESS:
			{
				if ( ! in->Stopping )
				{
					reply = __Cmd_Start_Process( usbbase, in, (PTR) msg );
				}
				else
				{
					// Not allowed to start new subtasks
					reply = TRUE;
				}
				break;
			}

			case CMD_STOP_PROCESS:
			{
				reply = __Cmd_Stop_Process( usbbase, in, (PTR) msg );
				break;
			}

			case CMD_FKT_DETACH:
			{
				reply = __Cmd_Fkt_Detach( usbbase, in, (PTR) msg );
				break;
			}

			default:
			{
				usbbase->usb_IExec->DebugPrintF( "%s:%04d: Unknown Cmd %ld\n", __FILE__, __LINE__, msg->cm_Command );
				reply = TRUE;
				break;
			}
		}

		if ( reply )
		{
			USBDEBUG( "myHandle_Cmds            : Reply Msg : TM %p : Task %p : Mask $%08lx", msg, msg->cm_Task, msg->cm_Mask );

			TASK_SIGNAL( msg->cm_Task, msg->cm_Mask );
		}
		else
		{
			USBDEBUG( "myHandle_Cmds            : Skipping Reply : TM %p : Task %p : Mask $%08lx", msg, msg->cm_Task, msg->cm_Mask  );
		}
	}
}

// --
