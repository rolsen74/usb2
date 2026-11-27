
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

SEC_CODE void
HID_Pointer_Main( struct USBBase *usbbase, PTR userdata, PTR in UNUSED )
{
	struct HID_ReportNode *rn;
	struct RealRequest *ioreq;
	struct USB2_WorkerHandoff handoff;

	U32 index;
	U32 wait;
	U32 mask;

	rn = userdata;
	wait = SIGBREAKF_CTRL_D | SIGBREAKF_CTRL_E | rn->IOReq_MsgPort.ump_Signal.sig_Signal_Mask;

	MEM_SET( &handoff, 0, sizeof( handoff ) );

	handoff.Entry = HID_Pointer_Main;
	handoff.Data = rn;
	handoff.Ports[0] = &rn->IOReq_MsgPort.ump_MsgPort;
	handoff.Ports[1] = &rn->Input_MsgPort.ump_MsgPort;
	handoff.Owners[0] = &rn->TaskAdr;

	while ( TRUE )
	{
		while ( ( ioreq = MSGPORT_GETMSG( &rn->IOReq_MsgPort ) ) )
		{
			if ( rn->Running )
			{
				if ( ( rn->HIDData->Driver_Mode != HID_DMode_Boot ) ||
					( ioreq->req_Public.io_Actual >= sizeof( struct USB2_BootMouse ) ) )
				{
					rn->Pointer_Data( usbbase, rn, ioreq );
				}
			}

			MSGPORT_PUTMSG( &rn->HIDData->Return_MsgPort, ioreq );
		}

		if ( ! rn->Running )
		{
			break;
		}

		mask = TASK_WAIT( wait );

		if ( mask & SIGBREAKF_CTRL_E )
		{
			handoff.Pending = mask;

			if ( USB2_PromoteWorker( usbbase, &handoff ) )
			{
				return;
			}
		}
	}

	for ( index = 0; index < 5; index++ )
	{
		HID_Mouse_Button( usbbase, rn, index, FALSE );
	}

	rn->TaskAdr = NULL;
}

// --
