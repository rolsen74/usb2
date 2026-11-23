
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

	enum IORStat state;
	S32			 current;

	while ( TRUE )
	{
		msg = MSGPORT_GETMSG ( &hn->hn_Abort_MsgPort );

		if ( ! msg )
		{
			break;
		}

		ioreq = (PTR)( (U8 *)msg - offsetof ( struct RealRequest, req_AbortMessage ) );
		fn	  = ioreq->req_Function;

		if ( FUNCTION_LOCK ( fn ) != LSTAT_Okay )
		{
			SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

			ioreq->req_AbortQueued = FALSE;

			IOREQUEST_UNLOCK ( ioreq );

			SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

			continue;
		}

		SEMAPHORE_OBTAIN ( &fn->fkt_Semaphore );

		SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

		ioreq->req_AbortQueued = FALSE;
		current				   = ( ioreq->req_AbortRequested ) && ( ioreq->req_AbortSubmission == ioreq->req_Submission );

		state = ioreq->req_PublicStat;

		IOREQUEST_UNLOCK ( ioreq );

		SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

		if ( ( current ) && ( state == IORS_HCD_Active ) )
		{
			ioreq->req_Public.io_Error = IOERR_ABORTED;

			HCD_TRANSFER_CHECK ( hn, ioreq, TRUE );
		}
		else if ( ( current ) && ( state == IORS_HCD_Queued ) )
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

		FUNCTION_UNLOCK ( fn );
	}
}

// --
SEC_CODE static void
DrainDetachedFunctions ( struct USBBase * usbbase, struct USB2_HCDNode * hn )
{
	struct RealFunctionNode * fn;
	struct RealRequest *	  ioreq;
	struct RealRequest *	  next;

	S32 pending;

	SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

	pending				 = hn->hn_DetachPending;
	hn->hn_DetachPending = FALSE;

	SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

	if ( ! pending )
	{
		return;
	}

	for ( ioreq = hn->hn_Active_Transfer_List.uh_Head; ioreq; ioreq = next )
	{
		next = Node_Next ( ioreq );
		fn	 = ioreq->req_Function;

		if ( ( fn->fkt_Detach ) && ( FUNCTION_LOCK ( fn ) == LSTAT_Okay ) )
		{
			SEMAPHORE_OBTAIN ( &fn->fkt_Semaphore );

			ioreq->req_Public.io_Error = USB2Err_Device_Detached;

			HCD_TRANSFER_CHECK ( hn, ioreq, TRUE );

			SEMAPHORE_RELEASE ( &fn->fkt_Semaphore );

			FUNCTION_UNLOCK ( fn );
		}
	}

	for ( ioreq = hn->hn_HUB_Interrupts.uh_Head; ioreq; ioreq = next )
	{
		next = Node_Next ( ioreq );

		if ( ioreq->req_Function->fkt_Detach )
		{
			NODE_REMNODE ( &hn->hn_HUB_Interrupts, ioreq );

			ioreq->req_Public.io_Error = USB2Err_Device_Detached;

			HCD_REPLY ( ioreq );
		}
	}

	if ( ( hn->hn_AcceptIO ) && ( hn->HCD_Functions.Retire_Detached ) )
	{
		hn->HCD_Functions.Retire_Detached ( hn );
	}
}

// --
