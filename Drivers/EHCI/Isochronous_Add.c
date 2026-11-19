
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
	struct EHCI_ITD **link;

	volatile U32 *hardware;
	U32 frame;
	U32 ioc;
	U32 split;
	U32 type;

	split = req->req_Function->fkt_Speed == USBSPEED_Full;
	type = split ? EHCI_LINK_SITD : 0;

	// Link prepared descriptors into future frame slots without modifying the interrupt QHs.
	for ( itd = req->req_HCD.EHCI.IsoHead; itd; itd = itd->Next )
	{
		frame = itd->Frame;
		link = & hn->hn_HCD.EHCI.IsoFrames[frame];
		hardware = & hn->hn_HCD.EHCI.FrameList[frame];

		if ( split )
		{
			ioc = itd->Next ? 0 : EHCI_SITD_IOC;
			itd->Split.Status = LE_SWAP32( EHCI_SITD_ACTIVE | ( itd->Length << 16 ) | ioc );
		}
		else
		{
			// Give split transactions first use of their assigned microframes.
			while ( ( *link ) && ( ( *link )->Hub ) )
			{
				hardware = & ( *link )->Link;
				link = & ( *link )->FrameNext;
			}

			ioc = itd->Next ? 0 : EHCI_ITD_IOC;
			itd->Transaction[itd->Microframe] = LE_SWAP32( EHCI_ITD_ACTIVE | ( itd->Length << 16 ) | ioc | EHCI_ITD_DATA );
		}

		itd->Link = *hardware;
		itd->FrameNext = *link;
		*link = itd;

		__asm__ volatile ( "sync" ::: "memory" );

		*hardware = LE_SWAP32( itd->Physical | type );
	}
}

// --
