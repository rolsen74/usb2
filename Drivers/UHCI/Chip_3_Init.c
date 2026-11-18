
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
UHCI_Chip_Init ( struct USB2_HCDNode * hn )
{
	struct UHCI_QH * qh;
	struct UHCI_TD * td;
	struct USBBase * usbbase;

	S32 retval;
	U32 cnt;

	usbbase = hn->hn_USBBase;
	retval = FALSE;

	TASK_NAME_ENTER ( "UHCI : UHCI_Chip_Init" );

	// --

	/*******************************************************************************
	 Initialize the frame array like seen at the following picture.

	 Frame List

	 .-----.
	 |  0  |  ---->  TD  ---->  QH                    BandWidth Reclaimation
	 |-----|                       \                .------------------------.
	 |  1  |  ---->  TD  ---->  QH                  |                        |
	 |-----|                         \              v                        ^
	 |  2  |  ---->  TD  ---->  QH    ---> QH  ---> QH ---> QH ---> QH ---> NULL
	 |-----|                         /                               |
	 | ... |  ---->  TD  ---->  QH                                  TD
	 |-----|                       /
	 | 1023|  ---->  TD  ---->  QH
	 '-----'

					 ISO        INTR       CTRL     CTRL    BULK    LAST
										   Low      Full

	*******************************************************************************/

	// -- Frame List

	hn->hn_HCD.UHCI.FrameList = MEMORY_ALLOC ( MEMID_HCD_4k, FALSE );

	if ( ! hn->hn_HCD.UHCI.FrameList )
	{
		goto bailout;
	}

	hn->hn_HCD.UHCI.FrameList_PhyAddr = ( (struct Mem_FreeNode *)hn->hn_HCD.UHCI.FrameList )->mfn_Addr;
	hn->hn_HCD.UHCI.FrameTD = MEM_ALLOCVEC( UHCI_FRAMELIST_COUNT * sizeof( struct UHCI_TD * ), TRUE );

	if ( ! hn->hn_HCD.UHCI.FrameTD )
	{
		goto bailout;
	}

	// -- Init List

	qh = UHCI_Get_QHBuffer ( hn, NULL );

	if ( ! qh )
	{
		goto bailout;
	}
	/* Keep an inactive TD on the anchor QH for PIIX controllers. */
	td = UHCI_Get_TDBuffer( hn );

	if ( ! td )
	{
		MEMORY_FREE( MEMID_UHCI_QH, qh, qh->pqh_self_phy );
		goto bailout;
	}

	qh->qh_elink = LE_SWAP32( td->ptd_self_phy );
	qh->pqh_first_td = td;

	hn->hn_HCD.UHCI.Last_Start = qh;
	hn->hn_HCD.UHCI.Last_End   = qh;
	for ( cnt = 0; cnt < UHCI_FRAMELIST_COUNT; cnt++ )
	{
		hn->hn_HCD.UHCI.FrameList[cnt] = LE_SWAP32( qh->pqh_self_phy | UHCI_PTR_QH );
	}

	// --

	retval = TRUE;

bailout:

	if ( ! retval )
	{
		UHCI_Chip_Deinit( hn );
	}

	TASK_NAME_LEAVE ();

	return ( retval );
}

// --
