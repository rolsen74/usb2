
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE static void
__Stopping( struct USBBase *usbbase, struct USB2_HCDNode *hn, struct intern *in )
{
	struct RealRequest *ioreq;

	U32 mask;
	U32 wait;
	S32 stopped;

	wait = SIGBREAKF_CTRL_D | in->Tick_MsgPort.ump_Signal.sig_Signal_Mask;
	stopped = FALSE;

	while ( TRUE )
	{
		__myHandle_Begin( usbbase, hn );

		while ( ( ioreq = NODE_REMHEAD( & hn->hn_HUB_Interrupts ) ) )
		{
			ioreq->req_Public.io_Error = USB2Err_Device_Detached;

			HCD_REPLY( ioreq );
		}

		if ( ( ! stopped ) && ( HCD_StopDMA( hn ) ) )
		{
			DrainStoppedTransfers( usbbase, hn, USB2Err_Device_Detached );

			stopped = TRUE;
		}

		if ( ( stopped ) && ( hn->hn_HUB_ASync.ua_Counter <= 0 ) && ( __Shutdown( usbbase, hn ) ) )
		{
			break;
		}

		mask = TASK_WAIT( wait );

		if ( ( mask & in->Tick_MsgPort.ump_Signal.sig_Signal_Mask ) && ( MSGPORT_GETMSG( & in->Tick_MsgPort ) ) )
		{
			in->Tick_TimeRequest.Time.Seconds = 1;
			in->Tick_TimeRequest.Time.Microseconds = 0;

			IO_SEND( & in->Tick_TimeRequest );
		}
	}
}

// --
