
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

// The caller has stopped DMA before dropping any active request.
SEC_CODE static void
DrainStoppedTransfers( struct USBBase *usbbase, struct USB2_HCDNode *hn, S32 error )
{
	struct USB2_EndPointNode *ep;
	struct RealRequest *ioreq;
	struct USB2_Node *node;
	struct USB2_Node *next;

	while ( ( ioreq = NODE_REMHEAD( & hn->hn_Active_Transfer_List ) ) )
	{
		ep = ioreq->req_EndPoint;

		// The active request pins the endpoint until its reply.
		ENDPOINT_LOCK( ep );

		ep->ep_Request_Active = NULL;
		ep->ep_Request_ActiveNext = NULL;

		if ( ioreq->req_TimerAdded )
		{
			IO_STOP( & ioreq->req_TimerIOReq );

			ioreq->req_TimerAdded = FALSE;
		}

		ioreq->req_Public.io_Actual = 0;
		ioreq->req_Public.io_Error = error;

		if ( hn->HCD_Functions.Transfer_Free )
		{
			hn->HCD_Functions.Transfer_Free( hn, ioreq );
		}

		HCD_REPLY( ioreq );

		ENDPOINT_UNLOCK( ep );
	}

	// A queue can remain without an active transfer while its interface is changing.
	SEMAPHORE_OBTAIN( & usbbase->usb_LockSemaphore );

	for ( node = usbbase->usb_IORequests.uh_Head ; node ; node = next )
	{
		next = Node_Next( node );
		ioreq = node->un_Data;

		if ( ( ioreq->req_PublicStat == IORS_HCD_Queued ) && ( ioreq->req_Function->fkt_Tier ) &&
			( ioreq->req_Function->fkt_HCD == hn ) )
		{
			NODE_REMNODE( & ioreq->req_EndPoint->ep_Request_Queue, ioreq );

			ioreq->req_Public.io_Actual = 0;
			ioreq->req_Public.io_Error = error;

			HCD_REPLY( ioreq );
		}
	}

	SEMAPHORE_RELEASE( & usbbase->usb_LockSemaphore );
}

// --

SEC_CODE static S32
__myHandle_Reset( struct USBBase *usbbase, struct USB2_HCDNode *hn )
{
	U32 port;

	HCD_CloseSubmissions( usbbase, hn );

	__myHandle_Begin( usbbase, hn );

	if ( ! HCD_StopDMA( hn ) )
	{
		usbbase->usb_IExec->DebugPrintF( "USB HCD %lu: halt failed, retaining DMA resources\n", hn->hn_HCDIndex );

		return ( FALSE );
	}

	DrainStoppedTransfers( usbbase, hn, USB2Err_Host_HostError );

	if ( ( hn->HCD_Functions.Chip_Reset ) && ( ! hn->HCD_Functions.Chip_Reset( hn ) ) )
	{
		usbbase->usb_IExec->DebugPrintF( "USB HCD %lu: reset failed\n", hn->hn_HCDIndex );

		return ( FALSE );
	}

	if ( hn->HCD_Functions.Chip_Deinit )
	{
		hn->HCD_Functions.Chip_Deinit( hn );
	}

	if ( ( hn->HCD_Functions.Chip_Init ) && ( ! hn->HCD_Functions.Chip_Init( hn ) ) )
	{
		usbbase->usb_IExec->DebugPrintF( "USB HCD %lu: initialization failed after reset\n", hn->hn_HCDIndex );

		return ( FALSE );
	}

	if ( hn->HCD_Functions.Chip_Start )
	{
		hn->hn_DMAActive = TRUE;

		if ( ! hn->HCD_Functions.Chip_Start( hn ) )
		{
			usbbase->usb_IExec->DebugPrintF( "USB HCD %lu: failed to start after reset\n", hn->hn_HCDIndex );

			return ( FALSE );
		}
	}

	SEMAPHORE_OBTAIN( & usbbase->usb_LockSemaphore );

	if ( hn->hn_HCDType == HCDTYPE_XHCI )
	{
		hn->hn_Generation++;

		for ( port = 1 ; ( port <= hn->hn_HUB_NumPorts ) && ( port < sizeof( hn->hn_RescanPort ) ) ; port++ )
		{
			hn->hn_RescanPort[port] = TRUE;
		}
	}

	hn->hn_AcceptIO = TRUE;

	SEMAPHORE_RELEASE( & usbbase->usb_LockSemaphore );

	ROOTHUB_HANDLE_CHANGE( hn );

	return ( TRUE );
}

// --
