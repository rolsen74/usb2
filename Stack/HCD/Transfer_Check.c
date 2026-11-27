
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

/*
** This function checks if the IORequest is done,
** and returns TRUE or FALSE.
**
** If the IORequest is done appropiate removale code is called.
*/

SEC_CODE S32
__HCD_Transfer_Check ( struct USBBase * usbbase, struct USB2_HCDNode * hn, struct RealRequest * ioreq, U32 Force )
{
	struct USB2_EndPointNode * ep;
	struct RealRequest * next;
	struct TimeVal now;

	U32 expected;
	U32 gap;
	S32						   remove;

	TASK_NAME_ENTER ( "HCD : __HCD_Transfer_Check : 1 :" );
	USBDEBUG ( "__HCD_Transfer_Check     : IOReq %p : Force %ld", ioreq, Force );

	if ( ! hn->hn_AcceptIO )
	{
		TASK_NAME_LEAVE ();

		return ( FALSE );
	}

#ifdef DO_DEBUG

	if ( HCD_VALID ( hn ) != VSTAT_Okay )
	{
		USBPANIC ( "__HCD_Transfer_Check     : Invalid hn %p", hn );
	}

	if ( IOREQUEST_VALID ( ioreq ) != VSTAT_Okay )
	{
		USBPANIC ( "__HCD_Transfer_Check     : Invalid ioreq %p", ioreq );
	}

#endif

	if ( Force )
	{
		remove = TRUE;
	}
	else
	{
		TASK_NAME_SET ( "HCD : __HCD_Transfer_Check : 2" );

		if ( hn->HCD_Functions.Transfer_Check )
		{
			remove = hn->HCD_Functions.Transfer_Check ( hn, ioreq );
		}
		else
		{
			USBDEBUG ( "__HCD_Transfer_Check     : Transfer_Check : Implement me" );
			remove = FALSE;
		}
	}

	if ( ! hn->hn_AcceptIO )
	{
		TASK_NAME_LEAVE ();

		return ( FALSE );
	}

	if ( remove )
	{
		TASK_NAME_SET ( "HCD : __HCD_Transfer_Check : 3" );

		USBDEBUG ( "__HCD_Transfer_Check     : Removing IOReq %p", ioreq );

		// the EP is no longer valid in IOReq after
		// HCD_Remove_Transfer, so get it now
		ep = ioreq->req_EndPoint;

		if ( ENDPOINT_LOCK ( ep ) != LSTAT_Okay )
		{
			TASK_NAME_LEAVE ();

			return ( FALSE );
		}

		// --

#ifdef DO_DEBUG

		if ( ENDPOINT_VALID ( ep ) != VSTAT_Okay )
		{
			USBPANIC ( "__HCD_Transfer_Check     : Invalid ep %p", ep );
		}

#endif

		// --

		TASK_NAME_SET ( "HCD : __HCD_Transfer_Check : 4" );

		// Remove node from Active TransferList
		NODE_REMNODE ( &hn->hn_Active_Transfer_List, ioreq );

		TASK_NAME_SET ( "HCD : __HCD_Transfer_Check : 5" );

		HCD_TRANSFER_REMOVE ( hn, ioreq );

		if ( ioreq->req_PublicStat == IORS_HCD_Active )
		{
			NODE_ADDHEAD ( &hn->hn_Active_Transfer_List, ioreq );

			ENDPOINT_UNLOCK ( ep );

			TASK_NAME_LEAVE ();

			return ( FALSE );
		}

		if ( ( ioreq->req_AltHeader ) && ( ioreq->req_Public.io_Error == USB2Err_NoError ) )
		{
			ioreq->req_AltApplied = TRUE;

			if ( ( hn->HCD_Functions.Reset_Interface ) && ( ! hn->HCD_Functions.Reset_Interface ( hn, ioreq->req_AltHeader ) ) )
			{
				ioreq->req_Public.io_Error = USB2Err_Host_HostError;
			}
		}

		if ( ( hn->HCD_Functions.Recover_Request ) && ( hn->HCD_Functions.Recover_Request( hn, ioreq ) ) )
		{
			ENDPOINT_UNLOCK( ep );

			TASK_NAME_LEAVE();

			return ( TRUE );
		}

		TASK_NAME_SET ( "HCD : __HCD_Transfer_Check : 6" );

		// Save the scheduled boundary before replying and releasing the request.
		expected = EHCI_FRINDEX_MASK + 1;

		if ( ( ! Force ) && ( hn->hn_IsEHCI ) && ( ep->ep_Type == EPATT_Type_Isochronous ) &&
			( ioreq->req_Public.io_Command == CMD_WRITE ) && ( ! ioreq->req_Public.io_Error ) &&
			( ep->ep_Request_Queue.uh_Head ) && ( ! ep->ep_Request_Active ) )
		{
			expected = ioreq->req_HCD.EHCI.IsoNextFrame;
		}

		HCD_REPLY ( ioreq );

		TASK_NAME_SET ( "HCD : __HCD_Transfer_Check : 7" );

		ioreq = NULL;

		// --
		// Start next IOReq if there are any queued
		HCD_RESTART_ENDPOINT ( hn, ep );

		next = ep->ep_Request_Active;

		if ( ( expected <= EHCI_FRINDEX_MASK ) && ( next ) && ( next->req_Public.io_Command == CMD_WRITE ) )
		{
			gap = ( next->req_HCD.EHCI.IsoFirstFrame - expected ) & EHCI_FRINDEX_MASK;

			if ( ( gap ) && ( gap <= EHCI_FRINDEX_MASK / 2 ) )
			{
				gap *= 125;

				if ( next->req_Public.io_IsoPacketLengths )
				{
					next->req_Public.io_Offset = gap;
				}

				hn->hn_HCD.EHCI.IsoOutGapCount++;
				hn->hn_HCD.EHCI.IsoOutGapUsecs += gap;
				hn->hn_HCD.EHCI.IsoOutGapMax = MAX ( hn->hn_HCD.EHCI.IsoOutGapMax, gap );

				usbbase->usb_ITimer->GetUpTime ( &now );

				// Keep missed playback time visible without logging each gap.
				if ( ( hn->hn_HCD.EHCI.IsoOutGapReports < 4 ) ||
					( (U32) ( now.Seconds - hn->hn_HCD.EHCI.IsoOutGapReportTime ) >= 30 ) )
				{
					usbbase->usb_IExec->DebugPrintF (
						"EHCI iso OUT gap: HCD %lu addr %lu ep %lu, %lu us with a write queued"
						" (%lu gaps, %lu us total, %lu us max since previous report)\n",
						(U32) hn->hn_HCDIndex, ep->ep_Function->fkt_Address, (U32) ep->ep_Number, gap,
						hn->hn_HCD.EHCI.IsoOutGapCount, hn->hn_HCD.EHCI.IsoOutGapUsecs, hn->hn_HCD.EHCI.IsoOutGapMax );

					hn->hn_HCD.EHCI.IsoOutGapReports++;
					hn->hn_HCD.EHCI.IsoOutGapReportTime = now.Seconds;
					hn->hn_HCD.EHCI.IsoOutGapCount = 0;
					hn->hn_HCD.EHCI.IsoOutGapUsecs = 0;
					hn->hn_HCD.EHCI.IsoOutGapMax = 0;
				}
			}
		}

		ENDPOINT_UNLOCK ( ep );

		TASK_NAME_SET ( "HCD : __HCD_Transfer_Check : 9" );
	}

	TASK_NAME_LEAVE ();

	return ( remove );
}

// --
