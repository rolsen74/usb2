
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
EHCI_Isochronous_Add ( struct USB2_HCDNode *hn, struct RealRequest *req )
{
	struct EHCI_ITD *itd;

	U32 microframe;
	U32 step;
	U32 frame;
	U32 ioc;
	U32 split;
	U32 type;

	split = req->req_Function->fkt_Speed == USBSPEED_Full;
	step = 1U << ( req->req_EndPoint->ep_Interval - 1 );
	type = 0;

	microframe = PCI_READLONG( hn->hn_HCD.EHCI.CapLength + EHCI_FRINDEX ) + 32;

	if ( split )
	{
		step *= 8;
		microframe = ( microframe + 7 ) & ~7U;
		type = EHCI_LINK_SITD;
	}

	// Link prepared descriptors into future frame slots without modifying the interrupt QHs.
	for ( itd = req->req_HCD.EHCI.IsoHead; itd; itd = itd->Next )
	{
		frame = ( microframe >> 3 ) & ( hn->hn_HCD.EHCI.FrameList_Size - 1 );
		itd->Frame = frame;

		if ( split )
		{
			ioc = itd->Next ? 0 : EHCI_SITD_IOC;
			itd->Split.Status = LE_SWAP32( EHCI_SITD_ACTIVE | ( itd->Length << 16 ) | ioc );
		}
		else
		{
			itd->Microframe = microframe & 7;
			ioc = itd->Next ? 0 : EHCI_ITD_IOC;
			itd->Transaction[itd->Microframe] = LE_SWAP32( EHCI_ITD_ACTIVE | ( itd->Length << 16 ) | ioc | EHCI_ITD_DATA );
		}

		itd->Link = hn->hn_HCD.EHCI.FrameList[frame];
		itd->LinkedNext = hn->hn_HCD.EHCI.IsoLinked;
		hn->hn_HCD.EHCI.IsoLinked = itd;

		__asm__ volatile ( "sync" ::: "memory" );

		hn->hn_HCD.EHCI.FrameList[frame] = LE_SWAP32( itd->Physical | type );
		microframe += step;
	}
}

// --
