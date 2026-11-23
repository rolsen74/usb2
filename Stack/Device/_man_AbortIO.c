
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
_manager_AbortIO ( struct DeviceManagerInterface * Self, struct IORequest * request )
{
	struct USBBase *	  usbbase;
	struct RealRequest *  ioreq;
	struct USB2_HCDNode * hn;

	usbbase = (PTR)Self->Data.LibBase;

	SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

	ioreq = FindLiveRequest ( usbbase, request );

	if ( ( ioreq ) && ( ioreq->req_PublicStat != IORS_User ) )
	{
		ioreq->req_AbortRequested  = TRUE;
		ioreq->req_AbortSubmission = ioreq->req_Submission;

		if ( ( ioreq->req_PublicStat != IORS_HCD_Pending ) && ( ! ioreq->req_AbortQueued )
			 && ( IOREQUEST_LOCK ( ioreq ) == LSTAT_Okay ) )
		{
			ioreq->req_AbortQueued = TRUE;
			hn					   = ioreq->req_Function->fkt_HCD;

			MSGPORT_PUTMSG ( &hn->hn_Abort_MsgPort, &ioreq->req_AbortMessage );
		}
	}

	SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );
}

// --
