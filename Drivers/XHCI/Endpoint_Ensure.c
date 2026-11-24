/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "XHCI.h"

// --

SEC_CODE S32
XHCI_Endpoint_Ensure ( struct USB2_HCDNode * hn, struct XHCI_Slot * slot, struct USB2_EndPointNode * ep, U32 direction )
{
	struct USBBase *	usbbase;
	struct XHCI_State * state;
	struct XHCI_Ring *	ring;

	U32 * input;
	U32 * endpoint;
	U32	  dci;
	U32	  type;
	U32	  interval;
	U32	  value;
	U32	  packet;
	U32	  burst;
	U32	  esit;
	U32	  average;

	usbbase = hn->hn_USBBase;
	state	= hn->hn_HCD.XHCI.State;
	dci		= ep->ep_Number * 2 + ( direction ? 1 : 0 );
	packet	= ep->ep_MaxPacketSize;
	burst	= ( slot->Speed == USBSPEED_High ) ? ( ep->ep_Transactions - 1 ) : 0;

	if ( ( ! dci ) || ( dci > 31 ) || ( ! packet ) || ( packet > 1024 ) || ( burst > 2 ) )
	{
		return ( FALSE );
	}

	average = packet * ( burst + 1 );
	esit = ( ep->ep_Type == EPATT_Type_Bulk ) ? 0 : average;

	if ( ( slot->Speed == USBSPEED_Super ) && ( ep->ep_Type == EPATT_Type_Isochronous ) )
	{
		esit = ep->ep_SS_IsoCapacity;

		if ( ! esit )
		{
			return ( FALSE );
		}
	}

	ring = &slot->EndPoints[dci];

	if ( ring->TRBs )
	{
		return ( TRUE );
	}

	if ( ! XHCI_Ring_Alloc ( hn, ring ) )
	{
		return ( FALSE );
	}

	MEM_SET ( slot->Input, 0, 4096 );
	MEM_COPY ( slot->Output, (U8 *)slot->Input + state->Context_Size, state->Context_Size );
	input	 = slot->Input;
	input[1] = LE_SWAP32 ( 1 | ( 1UL << dci ) );
	value	 = LE_SWAP32 ( input[state->Context_Size / 4] );
	value &= ~( 0x1fUL << 27 );
	value |= MAX ( slot->Context_Entries, dci ) << 27;
	input[state->Context_Size / 4] = LE_SWAP32 ( value );
	endpoint					   = (U32 *)( (U8 *)slot->Input + ( dci + 1 ) * state->Context_Size );

	switch ( ep->ep_Type )
	{
		case EPATT_Type_Bulk:
			type = direction ? 6 : 2;
			break;
		case EPATT_Type_Interrupt:
			type = direction ? 7 : 3;
			break;
		case EPATT_Type_Isochronous:
			type = direction ? 5 : 1;
			break;
		default:
			XHCI_Ring_Free ( hn, ring );

			return ( FALSE );
	}

	interval = 0;

	if ( ep->ep_Type != EPATT_Type_Bulk )
	{
		if ( slot->Speed >= USBSPEED_High )
		{
			interval = MAX ( 1, MIN ( ep->ep_Interval, 16 ) ) - 1;
		}
		else if ( ep->ep_Type == EPATT_Type_Isochronous )
		{
			interval = MAX ( 1, MIN ( ep->ep_Interval, 16 ) ) + 2;
		}
		else
		{
			value = MAX ( 1, ep->ep_Interval ) * 8;

			while ( ( 1UL << ( interval + 1 ) ) <= value )
			{
				interval++;
			}
		}
	}

	endpoint[0] = LE_SWAP32 ( interval << 16 );
	endpoint[1] = LE_SWAP32 ( ( packet << 16 ) | ( burst << 8 ) | ( type << 3 )
							  | ( ep->ep_Type == EPATT_Type_Isochronous ? 0 : ( 3 << 1 ) ) );
	endpoint[2] = LE_SWAP32 ( ring->Physical | 1 );
	endpoint[4] = LE_SWAP32 ( ( esit << 16 ) | average );

	if ( ! XHCI_Command ( hn, slot->Input_Physical, 0, 0, XHCI_TRB_TYPE ( XHCI_TRB_CONFIGURE_EP ) | ( slot->Slot_ID << 24 ),
						  NULL ) )
	{
		if ( state->Command_Done )
		{
			XHCI_Ring_Free ( hn, ring );
		}

		return ( FALSE );
	}

	slot->Context_Entries = MAX ( slot->Context_Entries, dci );

	return ( TRUE );
}

// --
