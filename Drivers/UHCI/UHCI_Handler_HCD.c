
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

/*
** Copyright (c) 1998 The NetBSD Foundation, Inc.
** All rights reserved.
**
** This code is derived from software contributed to The NetBSD Foundation
** by Lennart Augustsson (lennart@augustsson.net) at
** Carlstedt Research & Technology.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
** ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
** TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
** PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
** BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
** POSSIBILITY OF SUCH DAMAGE.
*/

// --

#include "usb2_all.h"
#include "UHCI.h"

// --

SEC_CODE void
UHCI_Handler_HCD ( struct USB2_HCDNode * hn, U32 mask )
{
	struct USBBase *	 usbbase;
	struct RealRequest * req;
	struct RealRequest * next;

	U32 port;
	U32 status;

	usbbase = hn->hn_USBBase;

	if ( hn->hn_HCD.UHCI.FatalStatus )
	{
		USBERROR( "UHCI controller fault, status %08lx", hn->hn_HCD.UHCI.FatalStatus );

		USB2_HCD_RequestReset( hn );

		return;
	}

	if ( ! hn->hn_AcceptIO )
	{
		return;
	}

	req		= hn->hn_Active_Transfer_List.uh_Head;

	while ( req )
	{
		struct RealFunctionNode * fn;

		fn = req->req_Function;

		next = Node_Next ( req );
		if ( FUNCTION_LOCK ( fn ) == LSTAT_Okay )
		{
			SEMAPHORE_OBTAIN ( &fn->fkt_Semaphore );
			HCD_TRANSFER_CHECK ( hn, req, FALSE );
			SEMAPHORE_RELEASE ( &fn->fkt_Semaphore );

			FUNCTION_UNLOCK ( fn );
		}
		if ( ! hn->hn_AcceptIO )
		{
			return;
		}

		req = next;
	}
	/* UHCI has no dedicated port-change interrupt. Poll on the HCD tick. */
	if ( mask == (U32)-1 )
	{
		for( port = 1; port <= hn->hn_HUB_NumPorts; port++ )
		{
			status = PCI_READWORD( UHCI_PORTSC( port ) );

			if ( ( status != 0xffff ) &&
				( ( status & ( UHCI_PORTSC_RD | UHCI_PORTSC_SUSP | UHCI_PORTSC_PE ) ) ==
				( UHCI_PORTSC_RD | UHCI_PORTSC_SUSP | UHCI_PORTSC_PE ) ) )
			{
				UHCI_Port_Clr_Suspend( hn, port );
			}
		}

		ROOTHUB_HANDLE_CHANGE( hn );
	}
}

// --
