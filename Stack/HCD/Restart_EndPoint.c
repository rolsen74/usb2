
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE void
__HCD_Restart_EndPoint ( struct USBBase * usbbase, struct USB2_HCDNode * hn, struct USB2_EndPointNode * ep )
{
	struct RealFunctionNode * fn;
	struct RealRequest *	  ioreq;

	fn = ep->ep_Function;

	SEMAPHORE_OBTAIN ( &fn->fkt_Semaphore );

	while ( ( ioreq = ep->ep_Request_Queue.uh_Head ) )
	{
		if ( ( fn->fkt_Detach ) || ( fn->fkt_FreeMe ) || ( ep->ep_Detach ) || ( ep->ep_FreeMe ) )
		{
			NODE_REMNODE ( &ep->ep_Request_Queue, ioreq );

			ioreq->req_Public.io_Error = USB2Err_Device_Detached;

			HCD_REPLY ( ioreq );
		}
		else if ( ( ep->ep_Interface ) && ( ep->ep_Interface->in_Parent->ih_Changing ) )
		{
			break;
		}

		else
		{
			if ( ! __HCD_Request_CanStart ( hn, ioreq ) )
			{
				break;
			}

			NODE_REMNODE ( &ep->ep_Request_Queue, ioreq );

			if ( ! HCD_ADD_REQUEST ( hn, ioreq ) )
			{
				break;
			}

			HCD_TRANSFER_REMOVE ( hn, ioreq );

			HCD_REPLY ( ioreq );
		}
	}

	SEMAPHORE_RELEASE ( &fn->fkt_Semaphore );
}

// --
