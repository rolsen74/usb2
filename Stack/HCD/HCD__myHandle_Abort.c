
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
CheckPortRecovery ( struct USBBase * usbbase, struct USB2_HCDNode * hn )
{
	struct RealFunctionNode * fn;
	struct USB2_Node * node;

	SEMAPHORE_OBTAIN ( &usbbase->usb_Fkt_Semaphore );

	SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

	hn->hn_PortRecoveryPending = FALSE;

	for ( node = usbbase->usb_Fkt_Header.uh_Head; node; node = Node_Next ( node ) )
	{
		fn = node->un_Data;

		if ( ( fn->fkt_HCD != hn ) || ( fn->fkt_PortRecovery != PORT_RECOVERY_DRAINING )
		  || ( ! fn->fkt_HUB_Data ) || ( fn->fkt_HCDGeneration != hn->hn_Generation ) )
		{
			continue;
		}

		if ( ( ! fn->fkt_Detach ) || ( fn->fkt_ActiveIOReq )
		  || ( ( hn->HCD_Functions.Function_Retired ) && ( ! hn->HCD_Functions.Function_Retired ( hn, fn->fkt_NotifyID ) ) ) )
		{
			hn->hn_PortRecoveryPending = TRUE;
		}
		else
		{
			fn->fkt_PortRecovery = PORT_RECOVERY_READY;
		}
	}

	SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

	SEMAPHORE_RELEASE ( &usbbase->usb_Fkt_Semaphore );
}

// --

SEC_CODE static void
DrainDetachedFunctions ( struct USBBase * usbbase, struct USB2_HCDNode * hn )
{
	struct RealFunctionNode * fn;
	struct RealRequest *	  ioreq;
	struct RealRequest *	  next;
	struct USB2_Node *		  node;
	struct USB2_Node *		  next_node;

	S32 pending;
	S32 recovering;

	SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

	pending = hn->hn_DetachPending;
	recovering = hn->hn_PortRecoveryPending;
	hn->hn_DetachPending = FALSE;

	SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

	if ( ( ! pending ) && ( ! recovering ) )
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

	if ( ! hn->hn_AcceptIO )
	{
		return;
	}

	if ( recovering )
	{
		SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

		for ( node = usbbase->usb_IORequests.uh_Head; node; node = next_node )
		{
			next_node = Node_Next ( node );
			ioreq = node->un_Data;
			fn = ioreq->req_Function;

			if ( ( fn->fkt_HCD == hn ) && ( fn->fkt_Detach )
			  && ( fn->fkt_PortRecovery == PORT_RECOVERY_DRAINING ) && ( ioreq->req_PublicStat == IORS_HCD_Queued ) )
			{
				NODE_REMNODE ( &ioreq->req_EndPoint->ep_Request_Queue, ioreq );

				ioreq->req_Public.io_Actual = 0;
				ioreq->req_Public.io_Error = USB2Err_Device_Detached;

				HCD_REPLY ( ioreq );
			}
		}

		SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );
	}

	if ( ( hn->HCD_Functions.Retire_Detached ) && ( ! hn->HCD_Functions.Retire_Detached ( hn ) ) )
	{
		if ( ( recovering ) && ( hn->hn_AcceptIO ) )
		{
			USB2_HCD_RequestReset ( hn );
		}

		return;
	}

	if ( ( hn->hn_AcceptIO ) && ( recovering ) )
	{
		CheckPortRecovery ( usbbase, hn );
	}
}

// --
