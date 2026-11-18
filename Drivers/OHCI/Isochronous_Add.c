
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "OHCI.h"

// --

SEC_CODE void
OHCI_Isochronous_Add ( struct USB2_HCDNode *hn, struct RealRequest *req )
{
	struct OHCI_ED *head;
	struct OHCI_ED *ed;
	struct OHCI_ITD *itd;

	U32 frame;
	U32 step;

	head = hn->hn_HCD.OHCI.Isoc_Head;
	ed = req->req_HCD.OHCI.ED_Header;
	frame = LE_SWAP16( hn->hn_HCD.OHCI.HCCA_Table->hcca_Frame_Number ) + 4;
	step = 1U << ( req->req_EndPoint->ep_Interval - 1 );

	for ( itd = req->req_HCD.OHCI.IsoHead; itd->Next; itd = itd->Next )
	{
		itd->itd_nextitd = LE_SWAP32( itd->Next->Physical );
		itd->itd_flags = LE_SWAP32( LE_SWAP32( itd->itd_flags ) | ( frame & 0xffff ) );
		frame += step;
	}

	ed->ed_nexted = head->ed_nexted;
	ed->ped_virt_next = head->ped_virt_next;
	head->ped_virt_next = ed;

	__asm__ volatile ( "sync" ::: "memory" );

	head->ed_nexted = LE_SWAP32( ed->ped_self_phy );
}

// --
