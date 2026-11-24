
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

	U32 index;
	U32 wait;

	rn = userdata;
	wait = SIGBREAKF_CTRL_D | rn->IOReq_MsgPort.ump_Signal.sig_Signal_Mask;

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

		TASK_WAIT( wait );
	}

	for ( index = 0; index < 5; index++ )
	{
		HID_Mouse_Button( usbbase, rn, index, FALSE );
	}

	rn->TaskAdr = NULL;
}

// --
