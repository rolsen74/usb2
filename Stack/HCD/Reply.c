
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

#if defined( DO_PANIC ) || defined( DO_ERROR ) || defined( DO_DEBUG ) || defined( DO_INFO )

SEC_CODE void
__HCD_Reply ( struct USBBase * usbbase UNUSED, struct RealRequest * ioreq, STR file UNUSED )

#else

SEC_CODE void
__HCD_Reply ( struct USBBase * usbbase UNUSED, struct RealRequest * ioreq )

#endif

{
	TASK_NAME_ENTER ( "__HCD_Reply : 1" );

	SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

	if ( IOREQUEST_LOCK ( ioreq ) == LSTAT_Okay )
	{
		//		usbbase->usb_IExec->DebugPrintF( "__HCD_Reply              : IOReq %p : Locks %2ld : Stat %ld : Task %p :\n",
		// ioreq, ioreq->req_Locks, ioreq->req_PublicStat, usbbase->usb_IExec->FindTask(NULL) );

		if ( ioreq->req_AbortQueued )
		{
			// The HCD owns both dispatch and completion of the abort message.
			usbbase->usb_IExec->Remove ( &ioreq->req_AbortMessage.mn_Node );

			ioreq->req_AbortQueued = FALSE;

			IOREQUEST_UNLOCK ( ioreq );
		}

		if ( ioreq->req_DestallTarget )
		{
			ENDPOINT_UNLOCK ( ioreq->req_DestallTarget );

			ioreq->req_DestallTarget = NULL;
		}

		IOREQUEST_ACTIVE_SUB ( ioreq );

		ioreq->req_PublicStat	  = IORS_User;
		ioreq->req_AbortRequested = FALSE;

		if ( usbbase->usb_Notify_IOWatchers )
		{
			USB2_Notify_IORequest ( usbbase, ioreq );
		}

		IOREQUEST_UNLOCK ( ioreq );

		usbbase->usb_IExec->ReplyMsg ( (PTR)&ioreq->req_Public );
	}
	else
	{
		USBPANIC ( "Failed to lock : IOReq %p : (%s)", ioreq, ( file ) ? file : "<NULL>" );
	}

	SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

	TASK_NAME_LEAVE ();
}

// --
