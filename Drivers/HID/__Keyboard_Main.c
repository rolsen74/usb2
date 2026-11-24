
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
__Keyboard_Main( struct USBBase *usbbase, PTR userdata, PTR in UNUSED )
{
	struct HID_ReportNode *rn;
	struct RealRequest *ioreq;

	U8 released[32];
	U32 wait;

	rn = userdata;
	wait = SIGBREAKF_CTRL_D | rn->IOReq_MsgPort.ump_Signal.sig_Signal_Mask |
		rn->Timer_MsgPort.ump_Signal.sig_Signal_Mask;

	while ( TRUE )
	{
		while ( ( ioreq = MSGPORT_GETMSG( &rn->IOReq_MsgPort ) ) )
		{
			if ( rn->Running )
			{
				if ( rn->HIDData->Driver_Mode == HID_DMode_Boot )
				{
					if ( ioreq->req_Public.io_Actual >= sizeof( struct USB2_BootKeyboard ) )
					{
						__Boot_Keyboard_Data( usbbase, rn, ioreq );
					}
				}
				else
				{
					__Keyboard_Data( usbbase, rn, ioreq );
				}
			}

			MSGPORT_PUTMSG( &rn->HIDData->Return_MsgPort, ioreq );
		}

		if ( ! rn->Running )
		{
			break;
		}

		if ( MSGPORT_GETMSG( &rn->Timer_MsgPort ) )
		{
			rn->Timer_Added = FALSE;

			if ( ! rn->Timer_IOReq.Request.io_Error )
			{
				__Keyboard_Timer( usbbase, rn );
			}
		}

		TASK_WAIT( wait );
	}

	MEM_SET( released, 0, sizeof( released ) );

	__Keyboard_Keys( usbbase, rn, released );

	rn->TaskAdr = NULL;
}

// --
