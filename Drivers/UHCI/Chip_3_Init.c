
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

SEC_CODE S32
UHCI_Chip_Init( struct USB2_HCDNode *hn )
{
	struct USBBase *usbbase;
	struct UHCI_QH *qh;
	struct UHCI_QH *next;
	struct UHCI_TD *td;

	U32 period;
	U32 phase;
	U32 frame;
	U32 slot;

	usbbase = hn->hn_USBBase;

	TASK_NAME_ENTER( "UHCI : UHCI_Chip_Init" );

	hn->hn_HCD.UHCI.FrameList = MEMORY_ALLOC( MEMID_HCD_4k, FALSE );

	if ( ! hn->hn_HCD.UHCI.FrameList )
	{
		goto failed;
	}

	hn->hn_HCD.UHCI.FrameList_PhyAddr = ( (struct Mem_FreeNode *) hn->hn_HCD.UHCI.FrameList )->mfn_Addr;

	hn->hn_HCD.UHCI.FrameTD = MEM_ALLOCVEC( UHCI_FRAMELIST_COUNT * sizeof( struct UHCI_TD * ), TRUE );

	hn->hn_HCD.UHCI.FrameLoad = MEM_ALLOCVEC( UHCI_FRAMELIST_COUNT * sizeof( U16 ), TRUE );

	hn->hn_HCD.UHCI.Periodic = MEM_ALLOCVEC( UHCI_PERIODIC_ANCHORS * sizeof( struct UHCI_QH * ), TRUE );

	if ( ( ! hn->hn_HCD.UHCI.FrameTD ) || ( ! hn->hn_HCD.UHCI.FrameLoad ) || ( ! hn->hn_HCD.UHCI.Periodic ) )
	{
		goto failed;
	}

	hn->hn_HCD.UHCI.BulkAnchor = UHCI_Get_QHBuffer( hn, NULL );

	hn->hn_HCD.UHCI.ControlAnchor = UHCI_Get_QHBuffer( hn, NULL );

	if ( ( ! hn->hn_HCD.UHCI.BulkAnchor ) || ( ! hn->hn_HCD.UHCI.ControlAnchor ) )
	{
		goto failed;
	}

	// Keep an inactive terminating TD for controllers that require one.
	td = UHCI_Get_TDBuffer( hn );

	if ( ! td )
	{
		goto failed;
	}

	qh = hn->hn_HCD.UHCI.BulkAnchor;
	qh->qh_elink = LE_SWAP32( td->ptd_self_phy );
	qh->pqh_first_td = td;
	hn->hn_HCD.UHCI.ControlAnchor->qh_hlink = LE_SWAP32( qh->pqh_self_phy | UHCI_PTR_QH );

	// A frame traverses its phase at each period, then control and bulk.
	for( period = 1; period <= UHCI_VFRAMELIST_COUNT; period <<= 1 )
	{
		for( phase = 0; phase < period; phase++ )
		{
			slot = period - 1 + phase;
			qh = UHCI_Get_QHBuffer( hn, NULL );

			if ( ! qh )
			{
				goto failed;
			}

			hn->hn_HCD.UHCI.Periodic[slot] = qh;

			if ( period == 1 )
			{
				next = hn->hn_HCD.UHCI.ControlAnchor;
			}
			else
			{
				next = hn->hn_HCD.UHCI.Periodic[period / 2 - 1 + phase % ( period / 2 )];
			}

			qh->qh_hlink = LE_SWAP32( next->pqh_self_phy | UHCI_PTR_QH );
		}
	}

	for( frame = 0; frame < UHCI_FRAMELIST_COUNT; frame++ )
	{
		slot = UHCI_VFRAMELIST_COUNT - 1 + frame % UHCI_VFRAMELIST_COUNT;
		qh = hn->hn_HCD.UHCI.Periodic[slot];
		hn->hn_HCD.UHCI.FrameList[frame] = LE_SWAP32( qh->pqh_self_phy | UHCI_PTR_QH );
	}

	TASK_NAME_LEAVE();

	return ( TRUE );

failed:
	UHCI_Chip_Deinit( hn );

	TASK_NAME_LEAVE();

	return ( FALSE );
}

// --
