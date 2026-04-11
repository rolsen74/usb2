
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

SEC_CODE void __Boot_Keyboard_Main( struct USBBase *usbbase, PTR userdata, PTR in UNUSED )
{
struct HID_ReportNode *rn;
struct RealRequest *ioreq;
// struct HIDData *hd;
U32 mask;
U32 wait;

	TASK_NAME_ENTER( "__Boot_Keyboard_Main" );

	rn = userdata;
//	hd = rn->HIDData;

	wait = SIGBREAKF_CTRL_D;
	wait |= rn->IOReq_MsgPort.ump_Signal.sig_Signal_Mask;
	wait |= rn->Timer_MsgPort.ump_Signal.sig_Signal_Mask;

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

				if ( ioreq->req_Public.io_Actual >= sizeof( struct USB2_BootKeyboard ))
				{
					__Boot_Keyboard_Data( usbbase, rn, ioreq );
				}
//				else
//				{
//					usbbase->usb_IExec->DebugPrintF( "__Boot_Keyboard_Main : Discard (Size %ld)\n", (S32) ioreq->req_Public.io_Actual );
//				}

				IO_SEND( ioreq );
			}
		}

		if ( mask & rn->Timer_MsgPort.ump_Signal.sig_Signal_Mask )
		{
			__Boot_Keyboard_Timer( usbbase, rn );
		}

		if ( mask & SIGBREAKF_CTRL_D )
		{
			// Used to break out of Wait so we can check ua_Counter
		}
	}

	rn->Running = FALSE;
	rn->TaskAdr = NULL;

// usbbase->usb_IExec->DebugPrintF( "__Boot_Keyboard_Main :: Leave : %s\n", rn->TaskName );

	TASK_NAME_LEAVE();
}

// --
