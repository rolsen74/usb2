
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

/*
** Copyright (c) 2001 The NetBSD Foundation, Inc.
** All rights reserved.
**
** This code is derived from software contributed to The NetBSD Foundation
** by Lennart Augustsson (lennart@augustsson.net).
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. All advertising materials mentioning features or use of this software
**    must display the following acknowledgement:
**        This product includes software developed by the NetBSD
**        Foundation, Inc. and its contributors.
** 4. Neither the name of The NetBSD Foundation nor the names of its
**    contributors may be used to endorse or promote products derived
**    from this software without specific prior written permission.
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
#include "EHCI.h"

// --

static SEC_CODE U32
EHCI_Isochronous_Time( struct USBBase *usbbase )
{
	struct TimeVal now;

	usbbase->usb_ITimer->GetUpTime( & now );

	return ( now.Seconds * 1000000U + now.Microseconds );
}

// --

SEC_CODE void
EHCI_Isochronous_Reap( struct USB2_HCDNode *hn, S32 halted )
{
	struct EHCI_ITD **link;
	struct EHCI_ITD *itd;
	struct USBBase *usbbase;

	U32 now;

	usbbase = hn->hn_USBBase;
	link = & hn->hn_HCD.EHCI.IsoRetired;
	now = 0;

	if ( ( *link ) && ( ! halted ) )
	{
		now = EHCI_Isochronous_Time( usbbase );

		// Newer descriptors are at the head. Keep cached links valid for two USB frames.
		while ( ( *link ) && ( (U32) ( now - ( *link )->RetiredAt ) < 2000 ) )
		{
			link = & ( *link )->Next;
		}
	}

	while ( ( itd = *link ) )
	{
		*link = itd->Next;

		MEMORY_FREE( MEMID_HCD_4k, itd, itd->Physical );
	}
}

// --

SEC_CODE void EHCI_Transfer_Free( struct USB2_HCDNode *hn, struct RealRequest *ioreq )
{
struct EHCI_TD *next;
struct EHCI_TD *td;
struct EHCI_QH *qh;
struct EHCI_ITD *itd;
U32 retired;

	struct USBBase *usbbase = hn->hn_USBBase;
	TASK_NAME_ENTER( "EHCI : EHCI_Transfer_Free" );

	EHCI_Isochronous_Reap( hn, FALSE );

	retired = 0;

	if ( ioreq->req_HCD.EHCI.IsoRetire )
	{
		retired = EHCI_Isochronous_Time( usbbase );
	}

	while ( ( itd = ioreq->req_HCD.EHCI.IsoHead ) )
	{
		ioreq->req_HCD.EHCI.IsoHead = itd->Next;

		if ( ioreq->req_HCD.EHCI.IsoRetire )
		{
			itd->RetiredAt = retired;
			itd->Next = hn->hn_HCD.EHCI.IsoRetired;
			hn->hn_HCD.EHCI.IsoRetired = itd;
		}
		else
		{
			MEMORY_FREE( MEMID_HCD_4k, itd, itd->Physical );
		}
	}

	ioreq->req_HCD.EHCI.IsoRetire = FALSE;

	qh = ioreq->req_HCD.EHCI.QH_Header;

	if ( ! qh )
	{
		// Not an error
		// If there is an early error, then this is NULL
		goto bailout;
	}

	ioreq->req_HCD.EHCI.QH_Header = NULL;

	td = qh->pqh_first_td;

	while( td )
	{
		next = td->ptd_virt_next;

		if ( td->td_buffer[0] )
		{
			if ( ! td->td_buffer[1] )
			{
				MEMORY_FREE( MEMID_HCD_4k, td->ptd_buffer_virt, td->ptd_buffer_phy );
			}
			else
			{
				MEMORY_FREE( MEMID_HCD_20k, td->ptd_buffer_virt, td->ptd_buffer_phy );
			}
		}

		MEMORY_FREE( MEMID_EHCI_TD, td, td->ptd_self_phy );

		td = next;
	}

	MEMORY_FREE( MEMID_EHCI_QH, qh, qh->pqh_self_phy );

bailout:

	TASK_NAME_LEAVE();

	return;
}

// --
