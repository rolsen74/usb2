
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
__myHandle_Abort ( struct USBBase * usbbase, struct USB2_HCDNode * hn )
{
	struct USB2_Header *	  queue;
	struct RealFunctionNode * fn;
	struct RealRequest *	  ioreq;
	struct RealRequest *	  queued;
	struct Message *		  msg;

	S32 current;

	while ( TRUE )
	{
		msg = MSGPORT_GETMSG ( &hn->hn_Abort_MsgPort );

		if ( ! msg )
		{
			break;
		}

		ioreq = (PTR)( (U8 *)msg - offsetof ( struct RealRequest, req_AbortMessage ) );
		fn	  = ioreq->req_Function;

		SEMAPHORE_OBTAIN ( &fn->fkt_Semaphore );

		SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

		ioreq->req_AbortQueued = FALSE;
		current				   = ( ioreq->req_AbortRequested ) && ( ioreq->req_AbortSubmission == ioreq->req_Submission );

		SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

		if ( ( current ) && ( ioreq->req_PublicStat == IORS_HCD_Active ) )
		{
			ioreq->req_Public.io_Error = IOERR_ABORTED;

			HCD_TRANSFER_CHECK ( hn, ioreq, TRUE );
		}
		else if ( ( current ) && ( ioreq->req_PublicStat == IORS_HCD_Queued ) )
		{
			queue = ( fn->fkt_Tier == 0 ) ? &hn->hn_HUB_Interrupts : &ioreq->req_EndPoint->ep_Request_Queue;

			for ( queued = queue->uh_Head; queued; queued = Node_Next ( queued ) )
			{
				if ( queued == ioreq )
				{
					NODE_REMNODE ( queue, ioreq );

					ioreq->req_Public.io_Error = IOERR_ABORTED;

					HCD_REPLY ( ioreq );
					break;
				}
			}
		}

		SEMAPHORE_RELEASE ( &fn->fkt_Semaphore );

		IOREQUEST_UNLOCK ( ioreq );
	}
}

// --
