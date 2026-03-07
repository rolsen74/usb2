
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
#include "OHCI.h"

// --

SEC_CODE S32 OHCI_Chip_Init( struct USB2_HCDNode *hn UNUSED )
{
//struct USBBase *usbbase;
// struct OHCI_QH *qh;
S32 retval;
// U32 slot;
// U32 cnt;
// U32 i;
// U32 j;
// U32 r;

//	struct USBBase *usbbase = hn->hn_USBBase;
	TASK_NAME_ENTER( "OHCI : OHCI_Chip_Init" );

// //	usbbase = hn->hn_USBBase;

 	retval = FALSE;

// 	// --

// 	USBDEBUG( "OHCI_Chip_Init           : 1 :" );
// 	USBDEBUG( "OHCI_Chip_Init           : 1 : ISlots %p", hn->hn_HCD.OHCI.FrameList_ISlots );

// 	/*
// 	 * Allocate the interrupt dummy QHs. These are arranged to give poll
// 	 * intervals that are powers of 2 times 1ms.
// 	 */

// 	for( cnt=0 ; cnt<255 ; cnt++ )
// 	{
// 		qh = OHCI_Get_QHBuffer( hn, NULL );

// //		USBDEBUG( "OHCI_Chip_Init           : 1 : QH %p", qh );

// 		if ( ! qh )
// 		{
// 			USBDEBUG( "OHCI_Chip_Init           : Error allocating memory" );
// 			goto bailout;
// 		}

// 		hn->hn_HCD.OHCI.FrameList_ISlots[ cnt ] = qh;
// 	}

// 	// --

// 	USBDEBUG( "OHCI_Chip_Init           : 2 : Link Pyramide QH's" );

// 	/*
// 	** Link ms Pyramide
// 	**
// 	**    1ms                             o0
// 	**
// 	**    2ms                     o1              o2
// 	**
// 	**    4ms                 o3      o4      o5      o6
// 	**
// 	**    8ms               o7  o8  o9  10  11  12  13  14
// 	**
// 	**   16ms             15 ..                       ..  30
// 	**
// 	**   32ms           31 ..                           ..  62
// 	**
// 	**   64ms         63 ..                               ..  126
// 	**
// 	**  128ms      127 ..                                   ..  254
// 	*/

// 	for( cnt=0 ; cnt<255 ; cnt++ )
// 	{
// 		qh = hn->hn_HCD.OHCI.FrameList_ISlots[ cnt ];

// 		if ( ! cnt )
// 		{
// 			// The last (1ms) QH terminates.
// 			qh->pqh_virt_next	= NULL;
// 			qh->qh_link			= OHCI_NULL;
// 		}
// 		else
// 		{
// 			// Otherwise the next QH has half the poll interval.
// 			qh->pqh_virt_next	= hn->hn_HCD.OHCI.FrameList_ISlots[ ((( cnt + 1 ) / 2 ) - 1 ) ];
// 			qh->qh_link			= LE_SWAP32( qh->pqh_virt_next->pqh_self_phy | OHCI_LINK_QH );
// 		}
// 	}

// 	// --
// 	// -- Periodic Frame List
// 	// Point the frame list at the last level (128ms).

// 	USBDEBUG( "OHCI_Chip_Init           : 3 : Build Periodic Frame List" );

// 	for( i=0 ; i<128 ; i++ )
// 	{
// 		r = OHCI_Reverse_Bits( i, OHCI_IPOLLRATES - 1 );

// 		slot = OHCI_IQHIDX( OHCI_IPOLLRATES, r );

// 		qh = hn->hn_HCD.OHCI.FrameList_ISlots[ slot ];

// 		for( j=i ; j<1024 ; j += 128 )
// 		{
// 			hn->hn_HCD.OHCI.FrameList[j] = LE_SWAP32( qh->pqh_self_phy | OHCI_LINK_QH );
// 		}
// 	}

// 	// --
// 	// Allocate dummy QH that starts the async list.

// 	USBDEBUG( "OHCI_Chip_Init           : 4 : Alloc QH Dummy Header" );

// 	qh = OHCI_Get_QHBuffer( hn, NULL );

// 	if ( ! qh )
// 	{
// 		USBDEBUG( "Error allocating memory" );
// 		goto bailout;
// 	}

// 	// Fill the QH
// 	qh->qh_link			= LE_SWAP32( qh->pqh_self_phy | OHCI_LINK_QH );
// 	qh->qh_ep1			= LE_SWAP32( OHCI_QH_SET_EPS( OHCI_QH_SPEED_HIGH ) | ( OHCI_QH_HRECL ));
// 	qh->qh_ep2			= 0;
// 	qh->qh_td_cur		= 0;
// 	qh->qh_td_next		= OHCI_NULL;
// 	qh->qh_td_alt		= OHCI_NULL;
// 	qh->qh_status		= 0;	//LE_SWAP32( OHCI_TD_HALTED );
// 	qh->pqh_virt_next	= qh;

// 	hn->hn_HCD.OHCI.ASync_Head = qh;

// 	// --

	retval = TRUE;

//bailout:

// 	USBDEBUG( "OHCI_Chip_Init           : 9 : retval %ld", retval );

	TASK_NAME_LEAVE();

	return( retval );
}

// --
