
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

// USB 2.0 sections 5.7.4 and 5.11.3. All times are in nanoseconds.
SEC_CODE static U32
OHCI_PeriodicTime( struct RealRequest *req )
{
	U32 bits;
	U32 overhead;

	// Worst case stuffing adds one bit for each six payload bits.
	bits = ( req->req_EndPoint->ep_MaxPacketSize * 56 + 5 ) / 6;

	if ( req->req_Function->fkt_Speed == USBSPEED_Low )
	{
		// Allow for the low-speed preamble, hub delays and handshake.
		return ( 80000 + ( bits * 2000 + 2 ) / 3 );
	}

	overhead = req->req_EndPoint->ep_Type == EPATT_Type_Isochronous ? 12000 : 15000;

	return ( overhead + ( bits * 250 + 2 ) / 3 );
}

// --

SEC_CODE S32
OHCI_Slot_Find( struct USB2_HCDNode *hn, struct RealRequest *req, U32 interval, U32 first )
{
	struct RealRequest *active;

	U32 limit;
	U32 cost;
	U32 best;
	U32 phase;
	U32 stride;
	U32 frame;
	U32 load;
	U32 peak;
	U32 mask;
	U32 slot;
	U32 bits;

	stride = MIN( interval, OHCI_NO_INTRS );
	active = req->req_EndPoint->ep_Request_Active;

	if ( active )
	{
		// The next isochronous batch uses the same endpoint and phase.
		req->req_HCD.OHCI.PeriodicCost = active->req_HCD.OHCI.PeriodicCost;
		req->req_HCD.OHCI.PeriodicMask = active->req_HCD.OHCI.PeriodicMask;
		req->req_HCD.OHCI.IsoFirstFrame = first & 0xffff;

		return ( TRUE );
	}

	cost = OHCI_PeriodicTime( req );
	best = OHCI_NO_INTRS;
	limit = 900000;

	for ( phase = 0 ; phase < stride ; phase++ )
	{
		peak = 0;

		for ( frame = phase ; frame < OHCI_NO_INTRS ; frame += stride )
		{
			load = hn->hn_HCD.OHCI.BandWidth[frame] + cost;
			peak = MAX( peak, load );
		}

		if ( peak <= limit )
		{
			limit = peak;
			best = phase;
		}
	}

	if ( best == OHCI_NO_INTRS )
	{
		return ( FALSE );
	}

	mask = 0;

	for ( frame = best ; frame < OHCI_NO_INTRS ; frame += stride )
	{
		hn->hn_HCD.OHCI.BandWidth[frame] += cost;
		mask |= 1UL << frame;
	}

	req->req_HCD.OHCI.PeriodicCost = cost;
	req->req_HCD.OHCI.PeriodicMask = mask;

	// The HCCA links to the leaves in bit-reversed frame order.
	slot = 0;
	bits = best;

	for ( phase = stride ; phase > 1 ; phase >>= 1 )
	{
		slot = ( slot << 1 ) | ( bits & 1 );
		bits >>= 1;
	}

	req->req_HCD.OHCI.Start_Slot = stride - 1 + slot;

	req->req_HCD.OHCI.IsoFirstFrame = ( first + ( ( best - first ) & ( stride - 1 ) ) ) & 0xffff;

	return ( TRUE );
}

// --
