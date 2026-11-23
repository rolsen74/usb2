
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

SEC_CODE void EHCI_Control_Remove( struct USB2_HCDNode *hn, struct RealRequest *ioreq )
{
struct RealFunctionNode *fn;
struct EHCI_TD *td;
struct EHCI_QH *last;
struct EHCI_QH *qh;

U32 token[3];
U32 caps;
U32 slot;

	struct USBBase *usbbase = hn->hn_USBBase;

	TASK_NAME_ENTER( "EHCI : EHCI_Control_Remove" );
//	hn->hn_USBBase->usb_IExec->DebugPrintF( "EHCI_Control_Remove : IOReq %p\n", ioreq );

	#if defined( DO_PANIC ) || defined( DO_ERROR ) || defined( DO_DEBUG ) || defined( DO_INFO )
	EHCI_Door_Bell_Init( hn, __FILE__ );
	#else
	EHCI_Door_Bell_Init( hn );
	#endif

	// --
	// Find the QH before our QH
	// so we can unlink it

	qh		= ioreq->req_HCD.EHCI.QH_Header;
	last	= hn->hn_HCD.EHCI.ASync_Head;

	while( last )
	{
		if ( last->pqh_virt_next == qh )
		{
			break;
		}
		else
		{
			last = last->pqh_virt_next;
		}
	}

	// Should never happen

	if ( ! last )
	{
		#ifdef DO_DEBUG
		hn->hn_USBBase->usb_IExec->DebugPrintF( "EHCI_Control_Remove : Yikes NULL Pointer\n" );
		#endif

		USB2_HCD_RequestReset( hn );

		TASK_NAME_LEAVE();

		return;
	}

	// --
	// Unlink it
	last->pqh_virt_next	= qh->pqh_virt_next;
	last->qh_link = qh->qh_link;

	// --
	// Tell HW to Flush async prefetch pointers
	#if defined( DO_PANIC ) || defined( DO_ERROR ) || defined( DO_DEBUG ) || defined( DO_INFO )
	if ( ! EHCI_Door_Bell_Wait( hn, __FILE__ ) )
	#else
	if ( ! EHCI_Door_Bell_Wait( hn ) )
	#endif
	{
		USB2_HCD_RequestReset( hn );

		TASK_NAME_LEAVE();

		return;
	}

	// --
	// its no longer active, but still controlled by HCD
	ioreq->req_PublicStat = IORS_HCD;

	fn = ioreq->req_Function;

	if ( ( ( ioreq->req_Public.io_Error == USB2Err_Host_Timeout ) ||
		   ( LE_SWAP32( qh->qh_status ) & EHCI_TD_HALTED ) ) &&
		 ( fn->fkt_DeviceDescriptor ) &&
		 ( LE_SWAP16( fn->fkt_DeviceDescriptor->VendorID ) == 0x0bda ) &&
		 ( LE_SWAP16( fn->fkt_DeviceDescriptor->DeviceID ) == 0x8152 ) )
	{
		// Snapshot after the controller releases the queue head.
		caps = LE_SWAP32( qh->qh_ep2 );
		td = qh->pqh_first_td;

		for( slot = 0; slot < 3; slot++ )
		{
			token[slot] = td ? LE_SWAP32( td->td_status ) : 0;
			td = td ? td->ptd_virt_next : NULL;
		}

		usbbase->usb_IExec->DebugPrintF( "RTL8152 EHCI EP0: addr %lu speed %lu max packet %lu TT hub %lu port %lu, QH $%08lx, qTDs $%08lx $%08lx $%08lx\n",
			(U32) fn->fkt_Address, (U32) fn->fkt_Speed, (U32) ioreq->req_EndPoint->ep_MaxPacketSize,
			EHCI_QH_GET_HUBA( caps ), EHCI_QH_GET_PORT( caps ), LE_SWAP32( qh->qh_status ),
			token[0], token[1], token[2] );
	}

	#if 0
	EHCI_DUMP_QH( hn, qh, TRUE );
	#endif

	TASK_NAME_LEAVE();
}

// --
