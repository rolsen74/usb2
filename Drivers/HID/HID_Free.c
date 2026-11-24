
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "HID.h"

// --

SEC_CODE static void
StopWorkers( struct USBBase *usbbase, struct HIDData *hd )
{
	struct HID_ReportNode *rn;
	struct ExecIFace *IExec;

	IExec = usbbase->usb_IExec;

	IExec->Disable();

	for ( rn = Header_Head( &hd->Reports ); rn; rn = Node_Next( rn ) )
	{
		if ( rn->TaskAdr )
		{
			rn->Running = FALSE;

			TASK_SIGNAL( rn->TaskAdr, SIGBREAKF_CTRL_D );
		}
	}

	IExec->Enable();

	ASYNC_WAIT( &hd->ASync_Drivers );

	if ( hd->Return_MsgPort.ump_StructID == ID_USB2_MP )
	{
		while ( MSGPORT_GETMSG( &hd->Return_MsgPort ) )
		{
			// The endpoint resource still owns each returned request.
		}
	}
}

// --

SEC_CODE void
Report_Free( struct USBBase *usbbase, struct HIDData *hd )
{
	struct HID_ReportNode *rn;

	U32 id;

	while ( ( rn = NODE_REMHEAD( &hd->Reports ) ) )
	{
		Node_Report_Free( usbbase, rn );
	}

	for ( id = 0; id < 256; id++ )
	{
		if ( hd->Layouts[id] )
		{
			MEM_FREEVEC( hd->Layouts[id] );

			hd->Layouts[id] = NULL;
		}
	}

	if ( hd->ReportBuffer )
	{
		MEM_FREEVEC( hd->ReportBuffer );

		hd->ReportBuffer = NULL;
	}

	hd->InputBufferSize = 0;
	hd->NumberedInput = FALSE;
}

// --

SEC_CODE void
HID_Free( struct USBBase *usbbase, struct HIDData *hd )
{
	StopWorkers( usbbase, hd );

	if ( hd->Register )
	{
		REGISTER_UNREGISTER( hd->Register );

		hd->Register = NULL;
	}

	MSGPORT_FREE( &hd->Return_MsgPort );

	MSGPORT_FREE( &hd->Timer_MsgPort );

	ASYNC_FREE( &hd->ASync_Drivers );

	Report_Free( usbbase, hd );
}

// --
