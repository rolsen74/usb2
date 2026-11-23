
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
XHCI_Endpoint_Drop ( struct USB2_HCDNode * hn, struct USB2_InterfaceHeader * ih )
{
	struct USBBase *			usbbase;
	struct XHCI_State *			state;
	struct XHCI_Slot *			slot;
	struct USB2_InterfaceNode * old;
	struct USB2_EndPointNode *	ep;

	U32 * input;
	U32	  dci;
	U32	  drop;
	U32	  entries;
	U32	  value;

	usbbase = hn->hn_USBBase;
	state	= hn->hn_HCD.XHCI.State;
	old		= ih->ih_Active;
	drop	= 0;
	entries = 1;

	slot = XHCI_Slot_Find ( hn, ih->ih_Function->fkt_NotifyID );

	if ( ( ! old ) || ( ! slot ) )
	{
		return ( TRUE );
	}

	for ( ep = old->in_EndPoints.uh_Head; ep; ep = Node_Next ( ep ) )
	{
		dci = ep->ep_Number * 2 + ( ep->ep_Direction == EPDIR_In );

		if ( slot->EndPoints[dci].TRBs )
		{
			drop |= 1UL << dci;
		}
	}

	if ( ! drop )
	{
		return ( TRUE );
	}

	for ( dci = 2; dci <= slot->Context_Entries; dci++ )
	{
		if ( ( slot->EndPoints[dci].TRBs ) && ( ! ( drop & ( 1UL << dci ) ) ) )
		{
			entries = dci;
		}
	}

	MEM_SET ( slot->Input, 0, 4096 );

	MEM_COPY ( slot->Output, (U8 *)slot->Input + state->Context_Size, state->Context_Size );

	input	 = slot->Input;
	input[0] = LE_SWAP32 ( drop );
	input[1] = LE_SWAP32 ( 1 );
	value	 = LE_SWAP32 ( input[state->Context_Size / 4] );
	value &= ~( 0x1fUL << 27 );
	input[state->Context_Size / 4] = LE_SWAP32 ( value | ( entries << 27 ) );

	if ( ! XHCI_Command ( hn, slot->Input_Physical, 0, 0, XHCI_TRB_TYPE ( XHCI_TRB_CONFIGURE_EP ) | ( slot->Slot_ID << 24 ),
						  NULL ) )
	{
		return ( FALSE );
	}

	// The controller no longer owns any of the dropped rings.
	for ( dci = 2; dci < 32; dci++ )
	{
		if ( drop & ( 1UL << dci ) )
		{
			XHCI_Ring_Free ( hn, &slot->EndPoints[dci] );
		}
	}

	slot->Context_Entries = entries;

	return ( TRUE );
}

// --
