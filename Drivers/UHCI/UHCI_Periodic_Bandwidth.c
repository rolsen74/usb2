
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "UHCI.h"

// --

SEC_CODE static U32
PacketCost( U32 bytes, U32 isoc, U32 low )
{
	U32 bits;

	// Charge worst-case bit stuffing and leave room for bus protocol overhead.
	bits = ( bytes * 56 + 5 ) / 6 + ( isoc ? 120 : 168 );

	return ( low ? 8 * bits + 64 : bits );
}

// --

SEC_CODE static S32
ReserveInterrupt( struct USB2_HCDNode *hn, struct RealRequest *req, struct UHCI_QH *qh )
{
	struct USB2_EndPointNode *ep;

	U16 *loads;
	U32 period;
	U32 phase;
	U32 best;
	U32 bestLoad;
	U32 worst;
	U32 frame;
	U32 cost;

	ep = req->req_EndPoint;
	loads = hn->hn_HCD.UHCI.FrameLoad;
	period = 1;

	while( ( ( period << 1 ) <= ep->ep_Interval ) && ( period < UHCI_VFRAMELIST_COUNT ) )
	{
		period <<= 1;
	}

	cost = PacketCost( ep->ep_MaxPacketSize, FALSE, req->req_Function->fkt_Speed == USBSPEED_Low );
	best = period;
	bestLoad = UHCI_PERIODIC_BITS + 1;

	for( phase = 0; phase < period; phase++ )
	{
		worst = 0;

		for( frame = phase; frame < UHCI_FRAMELIST_COUNT; frame += period )
		{
			worst = MAX( worst, loads[frame] + cost );
		}

		if ( worst < bestLoad )
		{
			best = phase;
			bestLoad = worst;
		}
	}

	if ( best == period )
	{
		return ( FALSE );
	}

	qh->pqh_period = period;
	qh->pqh_phase = best;
	qh->pqh_cost = cost;

	for( frame = best; frame < UHCI_FRAMELIST_COUNT; frame += period )
	{
		loads[frame] += cost;
	}

	return ( TRUE );
}

// --

SEC_CODE static S32
ReserveIsochronous( struct USB2_HCDNode *hn, struct RealRequest *req, struct UHCI_QH *qh )
{
	struct RealRequest *active;
	struct UHCI_QH *previous;
	struct UHCI_TD *td;

	U16 *loads;
	U32 now;
	U32 step;
	U32 offset;
	U32 frame;
	U32 after;

	active = req->req_EndPoint->ep_Request_Active;
	loads = hn->hn_HCD.UHCI.FrameLoad;
	now = PCI_READWORD( UHCI_FRNUM ) & UHCI_FRAME_MASK;
	step = 1U << ( req->req_EndPoint->ep_Interval - 1 );
	offset = UHCI_ISO_LEAD;

	if ( ( active ) && ( active != req ) && ( active->req_HCD.UHCI.QH_Header ) )
	{
		previous = active->req_HCD.UHCI.QH_Header;
		after = ( previous->pqh_last_frame + step - now ) & UHCI_FRAME_MASK;

		if ( after <= UHCI_FRAMELIST_MASK )
		{
			offset = MAX( offset, after );
		}
	}

	for( td = qh->pqh_first_td; td; td = td->ptd_virt_next )
	{
		if ( offset > UHCI_ISO_MAX_SPAN )
		{
			return ( FALSE );
		}

		frame = ( now + offset ) & UHCI_FRAME_MASK;

		if ( loads[frame & UHCI_FRAMELIST_MASK] + PacketCost( td->ptd_buffer_size, TRUE, FALSE ) > UHCI_PERIODIC_BITS )
		{
			return ( FALSE );
		}

		td->ptd_frame = frame;
		qh->pqh_last_frame = frame;
		offset += step;
	}

	for( td = qh->pqh_first_td; td; td = td->ptd_virt_next )
	{
		loads[td->ptd_frame & UHCI_FRAMELIST_MASK] += PacketCost( td->ptd_buffer_size, TRUE, FALSE );
	}

	qh->pqh_isoc = TRUE;

	return ( TRUE );
}

// --

SEC_CODE S32
UHCI_Periodic_Reserve( struct USB2_HCDNode *hn, struct RealRequest *req )
{
	struct UHCI_QH *qh;

	qh = req->req_HCD.UHCI.QH_Header;

	if ( req->req_EndPoint->ep_Type == EPATT_Type_Isochronous )
	{
		return ( ReserveIsochronous( hn, req, qh ) );
	}

	return ( ReserveInterrupt( hn, req, qh ) );
}

// --

SEC_CODE void
UHCI_Periodic_Release( struct USB2_HCDNode *hn, struct UHCI_QH *qh )
{
	struct UHCI_TD *td;

	U16 *loads;
	U32 frame;

	loads = hn->hn_HCD.UHCI.FrameLoad;

	if ( qh->pqh_isoc )
	{
		for( td = qh->pqh_first_td; td; td = td->ptd_virt_next )
		{
			loads[td->ptd_frame & UHCI_FRAMELIST_MASK] -= PacketCost( td->ptd_buffer_size, TRUE, FALSE );
		}
	}
	else if ( qh->pqh_period )
	{
		for( frame = qh->pqh_phase; frame < UHCI_FRAMELIST_COUNT; frame += qh->pqh_period )
		{
			loads[frame] -= qh->pqh_cost;
		}
	}
}

// --
