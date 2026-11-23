
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "Master.h"

// --
// Keep the port addresses because messages and timer requests reference them.
SEC_CODE static S32
Master_TransferPorts( struct ExecIFace *IExec, struct USBBase *usbbase, struct intern *in )
{
	struct USB2_MsgPort *ports[4];
	struct USB2_Signal signals[ sizeof( ports ) / sizeof( ports[0] ) ];
	struct Task *self;

	U32 mask;
	U32 i;

	ports[0] = &in->Cmd_MsgPort;
	ports[1] = &in->Notify_ReplyMsgPort;
	ports[2] = &in->Task_Shutdown_MsgPort;
	ports[3] = &in->Tick_MsgPort;
	mask = SIGBREAKF_CTRL_D;

	for ( i = 0; i < sizeof( signals ) / sizeof( signals[0] ); i++ )
	{
		signals[i].sig_StructID = 0;

		if ( ! TASK_ALLOCSIGNAL( &signals[i] ) )
		{
			while ( i )
			{
				i--;

				TASK_FREESIGNAL( &signals[i] );
			}

			USBERROR( "Master promotion could not allocate port signals" );

			return ( FALSE );
		}

		mask |= signals[i].sig_Signal_Mask;
	}

	self = IExec->FindTask( NULL );

	IExec->Disable();

	for ( i = 0; i < sizeof( ports ) / sizeof( ports[0] ); i++ )
	{
		ports[i]->ump_Signal = signals[i];
		ports[i]->ump_MsgPort.mp_SigTask = self;
		ports[i]->ump_MsgPort.mp_SigBit = signals[i].sig_Signal_Nr;
	}

	usbbase->usb_Master_Task.tn_TaskAdr = self;

	// Work already queued may only have signalled the old task.
	IExec->Signal( self, mask );

	IExec->Enable();

	return ( TRUE );
}

// --

SEC_CODE void
Master_Promote_Entry( void )
{
	struct MasterStartMsg *msg;
	struct ExecIFace *IExec;
	struct USBBase *usbbase;
	struct intern *in;

	IExec = USB2_GetExecIFace();

	msg = (PTR)( IExec->FindTask( NULL )->tc_UserData );

	usbbase = msg->usbbase;
	in = usbbase->usb_Master_Intern;

	if ( ! Master_TransferPorts( IExec, usbbase, in ) )
	{
		Master_ReplyStart( IExec, msg, FALSE );

		return;
	}

	TASK_SETPRI( 25 );

	Master_ReplyStart( IExec, msg, TRUE );

	// Let the old task finish using Master state before dispatching commands.
	do
	{
		IExec->Wait( SIGBREAKF_CTRL_E );
	}
	while ( ! in->PromotionReleased );

	TASK_SIGNAL( TASK_FIND(), SIGBREAKF_CTRL_E );

	__myProcess_Main( usbbase, in );

	usbbase->usb_Master_Task.tn_State = TASK_State_Stopping;

	__myProcess_Free( usbbase, in );

	Master_Finish( usbbase );
}

// --
