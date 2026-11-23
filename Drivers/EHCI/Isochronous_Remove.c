
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
	struct EHCI_ITD *last;
	struct EHCI_ITD **link;
	struct USBBase *usbbase;

	volatile U32 *hardware;
	U32 status;
	S32 restart;
	S32 paused;
	S32 cancel;

	usbbase = hn->hn_USBBase;
	last = req->req_HCD.EHCI.IsoHead;
	restart = FALSE;
	paused = FALSE;
	status = 0;
	cancel = req->req_Public.io_Error != USB2Err_NoError;

	if ( ! last )
	{
		return;
	}

	// A canceled high-speed read has no split transaction to drain from a hub.
	if ( ( cancel ) && ( ( req->req_Function->fkt_Speed != USBSPEED_High ) ||
		( req->req_Public.io_Command != CMD_READ ) ) )
	{
		EHCI_Periodic_Stop( hn );

		paused = TRUE;

		status = PCI_READLONG( hn->hn_HCD.EHCI.CapLength + EHCI_USBSTS );

		if ( ( status & EHCI_STS_PSS ) && ( ! ( status & EHCI_STS_HCH ) ) )
		{
			restart = TRUE;

			usbbase->usb_IExec->DebugPrintF(
				"EHCI: periodic schedule would not stop for addr %lu ep %lu, USBSTS $%08lx, restarting controller\n",
				req->req_Function->fkt_Address, (U32) req->req_EndPoint->ep_Number, status );

			EHCI_Chip_Stop( hn );

			status = PCI_READLONG( hn->hn_HCD.EHCI.CapLength + EHCI_USBSTS );
		}
	}

	if ( ( paused ) && ( status & EHCI_STS_PSS ) && ( ! ( status & EHCI_STS_HCH ) ) )
	{
		while ( last->Next )
		{
			last = last->Next;
		}

		// Keep DMA pages alive until the controller can be halted.
		last->Next = hn->hn_HCD.EHCI.IsoOrphan;
		hn->hn_HCD.EHCI.IsoOrphan = req->req_HCD.EHCI.IsoHead;
		req->req_HCD.EHCI.IsoHead = NULL;
		req->req_Public.io_Error = USB2Err_Host_HostError;
	}
	else
	{
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
		else if ( ! ( status & EHCI_STS_HCH ) )
		{
			EHCI_Periodic_Start( hn );
		}
	}

	if ( restart )
	{
		USB2_HCD_RequestReset( hn );
	}
}

// --
