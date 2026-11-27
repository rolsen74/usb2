
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

SEC_CODE U32
EHCI_Interrupt_Cost( U32 endpoint, U32 schedule )
{
	U32 packet;
	U32 mult;

	if ( EHCI_QH_GET_EPS( endpoint ) != EHCI_QH_SPEED_HIGH )
	{
		return ( 8 );
	}

	packet = EHCI_QH_GET_MPL( endpoint );
	mult = EHCI_QH_GET_MULT( schedule );

	return ( mult * ( ( ( packet * 7 + 5 ) / 6 + 64 + 59 ) / 60 ) );
}

// --

SEC_CODE S32
EHCI_Split_Interrupt_FitsSlot( U32 endpoint, U32 start )
{
	U32 packet;
	U32 stuffed_bits;
	U32 duration;

	if ( ( ! start ) || ( start & ~0x0fU ) || ( start & ( start - 1 ) ) )
	{
		return ( FALSE );
	}

	packet = EHCI_QH_GET_MPL( endpoint );
	stuffed_bits = ( packet * 56 + 5 ) / 6;

	switch ( EHCI_QH_GET_EPS( endpoint ) )
	{
		case EHCI_QH_SPEED_LOW:
		{
			if ( ( ! packet ) || ( packet > 8 ) )
			{
				return ( FALSE );
			}

			// USB 2.0 §5.11.3 low-speed IN bound, including hub setup and host delay.
			duration = 64060 + 2 * 333 + 1000 + ( 67667 * ( 31 + 10 * stuffed_bits ) + 999 ) / 1000;
			break;
		}

		case EHCI_QH_SPEED_FULL:
		{
			if ( ( ! packet ) || ( packet > 64 ) )
			{
				return ( FALSE );
			}

			// Full-speed interrupt transaction and host delay.
			duration = 9107 + 1000 + ( 8354 * ( 31 + 10 * stuffed_bits ) + 999 ) / 1000;
			break;
		}

		default:
		{
			return ( FALSE );
		}
	}

	// Budget the maximum TT think time (32 full-speed bit times) and the SOF token.
	duration += 2667;

	if ( start == 1 )
	{
		duration += 3000;
	}

	return ( duration <= 125000 );
}

// --

SEC_CODE static S32
AddBudget( U32 *usecs, U32 mask, U32 other, U32 cost )
{
	U32 index;

	other &= mask;

	for ( index = 0 ; other ; index++, other >>= 1 )
	{
		if ( other & 1 )
		{
			usecs[index] += cost;

			if ( usecs[index] > EHCI_ISO_USECS )
			{
				return ( FALSE );
			}
		}
	}

	return ( TRUE );
}

// --

SEC_CODE S32
EHCI_Periodic_Fits( struct USB2_HCDNode *hn, U32 frame, U32 mask, U32 cost, U32 hub, U32 split_start, S32 iso, S32 *split_overlap )
{
	struct EHCI_ITD *itd;
	struct EHCI_QH *qh;

	U32 slot;
	U32 endpoint;
	U32 schedule;
	U32 other_start;
	U32 other;
	U32 usecs[8];
	U32 index;

	slot = EHCI_Reverse_Bits( frame, EHCI_IPOLLRATES - 1 );
	slot = EHCI_IQHIDX( EHCI_IPOLLRATES, slot );

	for ( index = 0 ; index < 8 ; index++ )
	{
		usecs[index] = cost;
	}

	for ( qh = hn->hn_HCD.EHCI.FrameList_ISlots[slot] ; qh ; qh = qh->pqh_virt_next )
	{
		if ( qh->pqh_first_td )
		{
			endpoint = LE_SWAP32( qh->qh_ep1 );
			schedule = LE_SWAP32( qh->qh_ep2 );
			other_start = EHCI_QH_GET_SMASK( schedule );
			other = other_start | EHCI_QH_GET_CMASK( schedule );

			if ( ( split_overlap ) && ( EHCI_QH_GET_EPS( endpoint ) != EHCI_QH_SPEED_HIGH ) && ( other & mask ) )
			{
				*split_overlap = TRUE;
			}

			if ( ( hub ) && ( EHCI_QH_GET_EPS( endpoint ) != EHCI_QH_SPEED_HIGH ) &&
				( ( EHCI_QH_GET_HUBA( schedule ) ? EHCI_QH_GET_HUBA( schedule ) : EHCI_ROOT_TT ) == hub ) &&
				( other & mask ) )
			{
				// A TT can return results within overlapping C-masks if the
				// downstream interrupt transactions fit in separate slots.
				if ( ( ! split_start ) || ( split_start & other_start ) ||
					( ! EHCI_Split_Interrupt_FitsSlot( endpoint, other_start ) ) )
				{
					return ( FALSE );
				}
			}

			if ( ! AddBudget( usecs, mask, other, EHCI_Interrupt_Cost( endpoint, schedule ) ) )
			{
				return ( FALSE );
			}
		}
	}

	for ( itd = hn->hn_HCD.EHCI.IsoFrames[frame] ; itd ; itd = itd->FrameNext )
	{
		// Treat each hub as one TT unless its per-port topology is known.
		if ( ( hub ) && ( itd->Hub == hub ) && ( ( iso ) || ( itd->Mask & mask ) ) )
		{
			return ( FALSE );
		}

		if ( ( split_overlap ) && ( itd->Hub ) && ( itd->Mask & mask ) )
		{
			*split_overlap = TRUE;
		}

		if ( ! AddBudget( usecs, mask, itd->Mask, itd->Usecs ) )
		{
			return ( FALSE );
		}
	}

	return ( TRUE );
}

// --
