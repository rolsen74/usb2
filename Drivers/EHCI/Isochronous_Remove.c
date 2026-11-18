
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
	struct USBBase *usbbase;

	U32 status;
	S32 restart;

	usbbase = hn->hn_USBBase;

	if ( ! req->req_HCD.EHCI.IsoHead )
	{
		return;
	}

	EHCI_Periodic_Stop( hn );

	status = PCI_READLONG( hn->hn_HCD.EHCI.CapLength + EHCI_USBSTS );
	restart = FALSE;

	if ( ( status & EHCI_STS_PSS ) && ( ! ( status & EHCI_STS_HCH ) ) )
	{
		restart = TRUE;

		EHCI_Chip_Stop( hn );

		status = PCI_READLONG( hn->hn_HCD.EHCI.CapLength + EHCI_USBSTS );
	}

	if ( ( status & EHCI_STS_PSS ) && ( ! ( status & EHCI_STS_HCH ) ) )
	{
		// Keep buffers owned by the controller alive until a successful halt.
		hn->hn_HCD.EHCI.IsoOrphan = req->req_HCD.EHCI.IsoHead;
		req->req_HCD.EHCI.IsoHead = NULL;
		req->req_Public.io_Error = USB2Err_Host_HostError;
	}
	else
	{
		while ( ( itd = hn->hn_HCD.EHCI.IsoLinked ) )
		{
			hn->hn_HCD.EHCI.FrameList[itd->Frame] = itd->Link;
			hn->hn_HCD.EHCI.IsoLinked = itd->LinkedNext;
		}

		__asm__ volatile ( "sync" ::: "memory" );

		if ( ! ( status & EHCI_STS_HCH ) )
		{
			EHCI_Periodic_Start( hn );
		}
	}

	if ( restart )
	{
		TASK_SIGNAL( hn->hn_Task->tn_TaskAdr, hn->HCD_Restart_Chip.sig_Signal_Mask );
	}
}

// --
