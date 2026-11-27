
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

SEC_CODE S32
EHCI_Slot_Find( struct USB2_HCDNode *hn, struct RealRequest *ioreq )
{
	struct EHCI_QH *qh;

	U32 endpoint;
	U32 schedule;
	U32 interval;
	U32 microframes;
	U32 frames;
	U32 level;
	U32 frame;
	U32 phase;
	U32 start;
	U32 mask;
	U32 complete;
	U32 split_start;
	U32 phases;
	U32 hub;
	U32 cost;
	U32 limit;
	U32 index;
	U32 slot;
	U32 fallback_slot;
	U32 fallback_schedule;
	S32 split;
	S32 overlap;

	qh = ioreq->req_HCD.EHCI.QH_Header;
	endpoint = LE_SWAP32( qh->qh_ep1 );
	schedule = LE_SWAP32( qh->qh_ep2 );
	interval = ioreq->req_EndPoint->ep_Interval;
	split = EHCI_QH_GET_EPS( endpoint ) != EHCI_QH_SPEED_HIGH;

	if ( ( ! interval ) || ( ( ! split ) && ( interval > 16 ) ) )
	{
		return ( FALSE );
	}

	hub = EHCI_QH_GET_HUBA( schedule );

	if ( split )
	{
		hub = hub ? hub : EHCI_ROOT_TT;
		microframes = 8;
		frames = interval;
		phases = 4;
	}
	else
	{
		microframes = 1U << ( interval - 1 );
		frames = MAX( microframes / 8, 1 );
		phases = MIN( microframes, 8 );
	}

	// The existing periodic tree repeats every 128 frames.
	for ( level = EHCI_IPOLLRATES - 1 ; ( 1U << level ) > frames ; level-- )
	{
	}

	frames = 1U << level;
	cost = EHCI_Interrupt_Cost( endpoint, schedule );
	limit = hn->hn_HCD.EHCI.IsoScheduled ? hn->hn_HCD.EHCI.FrameList_Size : ( 1U << ( EHCI_IPOLLRATES - 1 ) );

	fallback_slot = 0;
	fallback_schedule = 0;

	for ( frame = 0 ; frame < frames ; frame++ )
	{
		for ( phase = 0 ; phase < phases ; phase++ )
		{
			mask = 1U << phase;
			complete = 0;
			split_start = 0;
			overlap = FALSE;

			if ( split )
			{
				complete = 0x1cU << phase;

				if ( EHCI_Split_Interrupt_FitsSlot( endpoint, mask ) )
				{
					split_start = mask;
				}
			}
			else
			{
				for ( start = phase + microframes ; start < 8 ; start += microframes )
				{
					mask |= 1U << start;
				}
			}

			for ( index = frame ; index < limit ; index += frames )
			{
				if ( ! EHCI_Periodic_Fits( hn, index, mask | complete, cost, hub, split_start, FALSE,
					split ? &overlap : NULL ) )
				{
					break;
				}
			}

			if ( index < limit )
			{
				continue;
			}

			slot = EHCI_IQHIDX( level + 1, EHCI_Reverse_Bits( frame, level ) );

			if ( ! overlap )
			{
				ioreq->req_HCD.EHCI.Start_Slot = slot;
				qh->qh_ep2 = LE_SWAP32( schedule | EHCI_QH_SET_SMASK( mask ) | EHCI_QH_SET_CMASK( complete ) );

				return ( TRUE );
			}

			// Prefer separate split windows across TTs when the poll interval allows it.
			if ( ! fallback_schedule )
			{
				fallback_slot = slot;
				fallback_schedule = schedule | EHCI_QH_SET_SMASK( mask ) | EHCI_QH_SET_CMASK( complete );
			}
		}
	}

	if ( fallback_schedule )
	{
		ioreq->req_HCD.EHCI.Start_Slot = fallback_slot;
		qh->qh_ep2 = LE_SWAP32( fallback_schedule );

		return ( TRUE );
	}

	return ( FALSE );
}

// --
