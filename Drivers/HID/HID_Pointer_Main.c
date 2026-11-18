
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

SEC_CODE void HID_Pointer_Main( struct USBBase *usbbase, PTR userdata, PTR in UNUSED )
{
struct HID_ReportNode *rn;
struct RealRequest *ioreq;
//struct HIDData *hd;
U32 mask;
U32 wait;

	TASK_NAME_ENTER( "HID_Pointer_Main" );

	rn = userdata;
//	hd = rn->HIDData;

	wait = SIGBREAKF_CTRL_D;

	wait |= rn->IOReq_MsgPort.ump_Signal.sig_Signal_Mask;

	rn->Running = TRUE;

	while( rn->Running )
	{
		mask = TASK_WAIT( wait );

		if ( mask &	rn->IOReq_MsgPort.ump_Signal.sig_Signal_Mask )
		{
			while( TRUE )
			{
				ioreq = MSGPORT_GETMSG( & rn->IOReq_MsgPort );

				if ( ! ioreq )
				{
					break;
				}

				rn->Pointer_Data( usbbase, rn, ioreq );

				IO_SEND( ioreq );
			}
		}

		if ( mask & SIGBREAKF_CTRL_D )
		{
			// Used to break out of Wait so we can check ua_Counter
		}
	}

	if ( rn->Tablet_Buttons )
	{
		Tablet_Buttons ( usbbase, rn, 0 );
	}

	rn->Running = FALSE;
	rn->TaskAdr = NULL;

	TASK_NAME_LEAVE();
}

// --
