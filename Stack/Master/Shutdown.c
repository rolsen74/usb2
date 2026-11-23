
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE void
__Master_Shutdown( struct USBBase *usbbase )
{
	struct USB2_TaskNode *tn;
	struct ExecIFace *IExec;
	struct Task *self;

	S32 signal;
	S32 running;
	U32 mask;

	tn = &usbbase->usb_Master_Task;
	IExec = usbbase->usb_IExec;

	self = IExec->FindTask( NULL );

	signal = IExec->AllocSignal( -1 );

	mask = ( signal >= 0 ) ? ( 1UL << signal ) : SIGBREAKF_CTRL_F;

	IExec->Disable();

	running = ( tn->tn_TaskAdr != NULL );

	if ( running )
	{
		usbbase->usb_Master_MsgPort = NULL;
		tn->tn_Parent = self;
		tn->tn_Mask = mask;

		IExec->Signal( tn->tn_TaskAdr, SIGBREAKF_CTRL_C );
	}

	IExec->Enable();

	while ( running )
	{
		IExec->Wait( mask );

		// A break signal can be pending when no private signal is available.
		IExec->Disable();

		running = ( tn->tn_TaskAdr != NULL );

		IExec->Enable();
	}

	if ( signal >= 0 )
	{
		IExec->FreeSignal( signal );
	}
}

// --
