/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "XHCI.h"

// --

SEC_CODE void
XHCI_Handler_HCD ( struct USB2_HCDNode * hn, U32 mask UNUSED )
{
	struct USBBase *		  usbbase;
	struct RealRequest *	  req;
	struct RealRequest *	  next;
	struct RealFunctionNode * fn;

	usbbase = hn->hn_USBBase;

	XHCI_Event_Poll ( hn );

	for ( req = hn->hn_Active_Transfer_List.uh_Head; req; req = next )
	{
		next = Node_Next ( req );
		fn	 = req->req_Function;
		if ( FUNCTION_LOCK ( fn ) == LSTAT_Okay )
		{
			SEMAPHORE_OBTAIN ( &fn->fkt_Semaphore );
			HCD_TRANSFER_CHECK ( hn, req, FALSE );
			SEMAPHORE_RELEASE ( &fn->fkt_Semaphore );

			FUNCTION_UNLOCK ( fn );
		}
	}

	ROOTHUB_HANDLE_CHANGE ( hn );
}

// --
