
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "EHCI.h"

// --

SEC_CODE void
EHCI_Isochronous_Remove ( struct USB2_HCDNode *hn, struct RealRequest *req )
{
	struct EHCI_ITD *itd;
	struct EHCI_ITD **link;
	struct USBBase *usbbase;

	volatile U32 *hardware;
	S32 paused;
	S32 cancel;

	usbbase = hn->hn_USBBase;
	paused = FALSE;
	cancel = req->req_Public.io_Error != USB2Err_NoError;

	if ( ! req->req_HCD.EHCI.IsoHead )
	{
		return;
	}

	// A canceled high-speed read has no split transaction to drain from a hub.
	if ( ( cancel ) && ( ( req->req_Function->fkt_Speed != USBSPEED_High ) ||
		( req->req_Public.io_Command != CMD_READ ) ) )
	{
		paused = TRUE;

		if ( ! EHCI_Periodic_Stop( hn ) )
		{
			USB2_HCD_RequestReset( hn );

			return;
		}
	}

	for ( itd = req->req_HCD.EHCI.IsoHead; itd; itd = itd->Next )
	{
		link = & hn->hn_HCD.EHCI.IsoFrames[itd->Frame];
		hardware = & hn->hn_HCD.EHCI.FrameList[itd->Frame];

		while ( ( *link ) && ( *link != itd ) )
		{
			hardware = & ( *link )->Link;
			link = & ( *link )->FrameNext;
		}

		if ( *link )
		{
			*link = itd->FrameNext;
			*hardware = itd->Link;
		}
	}

	__asm__ volatile ( "sync" ::: "memory" );

	if ( ! paused )
	{
		if ( cancel )
		{
			// Let cached transactions finish before reporting the canceled read.
			// Other endpoints can continue using the periodic schedule.
			MISC_MICRODELAY( 2000 );
		}
		else
		{
			// The controller can still have completed descriptor links cached.
			req->req_HCD.EHCI.IsoRetire = TRUE;
		}
	}
	else
	{
		EHCI_Periodic_Start( hn );
	}
}

// --
