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

SEC_CODE struct XHCI_Slot *
XHCI_Slot_Create ( struct USB2_HCDNode * hn, struct RealFunctionNode * fn )
{
	struct USBBase * usbbase;
	struct XHCI_State * state;
	struct XHCI_Slot * slot;
	struct RealFunctionNode * parent;
	struct RealFunctionNode * cursor;
	struct RealFunctionNode * tt_hub;
	struct RealRequest * active;
	struct XHCI_Slot * old;

	U32 * input;
	U32 * output;
	U32 * dcbaa;
	U32 port;
	U32 speed;
	U32 packet;
	U32 slot_id;
	U32 index;
	U32 busy;
	U32 route;
	U32 depth;
	U32 tt_port;
	U32 tt_slot;

	usbbase = hn->hn_USBBase;
	state = hn->hn_HCD.XHCI.State;
	parent = fn->fkt_Parent;
	cursor = fn;
	route = 0;
	depth = 0;
	tt_hub = NULL;
	tt_port = 0;
	tt_slot = 0;

	if ( ! parent )
	{
		return ( NULL );
	}

	while ( ( cursor->fkt_Parent ) && ( cursor->fkt_Parent->fkt_Tier != 0 ) )
	{
		if ( ( depth >= 5 ) || ( ! cursor->fkt_PortNr ) )
		{
			return ( NULL );
		}

		if ( ( fn->fkt_Speed <= USBSPEED_Full ) &&
			( cursor->fkt_Parent->fkt_Speed == USBSPEED_High ) && ( ! tt_hub ) )
		{
			tt_hub = cursor->fkt_Parent;
			tt_port = cursor->fkt_PortNr;
		}

		if ( ( cursor->fkt_PortNr > 15 ) && ( cursor->fkt_Parent->fkt_Speed >= USBSPEED_Super ) )
		{
			return ( NULL );
		}

		route = ( route << 4 ) | MIN ( cursor->fkt_PortNr, 15 );
		cursor = cursor->fkt_Parent;
		depth++;
	}

	port = cursor->fkt_PortNr;

	if ( ( ! port ) || ( port > state->Max_Ports ) )
	{
		return ( NULL );
	}

	if ( parent->fkt_Tier != 0 )
	{
		if ( ! XHCI_Hub_Ensure ( hn, parent ) )
		{
			return ( NULL );
		}
	}

	if ( tt_hub )
	{
		if ( ! XHCI_Hub_Ensure ( hn, tt_hub ) )
		{
			return ( NULL );
		}

		for ( index = 1; index <= state->Max_Slots; index++ )
		{
			if ( ( state->Slots[index] ) && ( state->Slots[index]->Function == tt_hub ) )
			{
				tt_slot = index;
				break;
			}
		}

		if ( ! tt_slot )
		{
			return ( NULL );
		}

	}

	for ( index = 1; ( parent->fkt_Tier == 0 ) && ( index <= state->Max_Slots ); index++ )
	{
		old = state->Slots[index];

		if ( ( ! old ) || ( old->Root_Port != port ) )
		{
			continue;
		}

		busy = FALSE;

		for ( active = hn->hn_Active_Transfer_List.uh_Head; active; active = Node_Next ( active ) )
		{
			if ( active->req_HCD.XHCI.Slot == old )
			{
				busy = TRUE;
				break;
			}
		}

		if ( busy )
		{
			return ( NULL );
		}

		if ( ! XHCI_Command ( hn, 0, 0, 0,
			XHCI_TRB_TYPE ( XHCI_TRB_DISABLE_SLOT ) | ( old->Slot_ID << 24 ), NULL ) )
		{
			return ( NULL );
		}

		( (U32 *)state->DCBAA )[old->Slot_ID * 2] = 0;
		XHCI_Slot_Free ( hn, old );
	}

	if ( parent->fkt_Tier == 0 )
	{
		speed = XHCI_PORT_SPEED ( PCI_READLONG ( state->Operational + XHCI_PORTSC ( port ) ) );
	}
	else
	{
		switch ( fn->fkt_Speed )
		{
			case USBSPEED_Low:
				speed = 2;
				break;
			case USBSPEED_Full:
				speed = 1;
				break;
			case USBSPEED_High:
				speed = 3;
				break;
			case USBSPEED_Super:
				speed = 4;
				break;
			default:
				return ( NULL );
		}
	}

	if ( ! speed )
	{
		return ( NULL );
	}

	packet = ( speed >= 4 ) ? 512 : ( speed == 3 ) ? 64 : 8;
	slot = MEM_ALLOCVEC ( sizeof ( *slot ), TRUE );

	if ( ! slot )
	{
		return ( NULL );
	}

	if ( ! XHCI_Ring_Alloc ( hn, &slot->EndPoints[1] ) )
	{
		goto failed;
	}

	slot->Input = MEMORY_ALLOC ( MEMID_HCD_4k, FALSE );

	if ( ! slot->Input )
	{
		goto failed;
	}

	slot->Input_Physical = ( (struct Mem_FreeNode *)slot->Input )->mfn_Addr;
	slot->Output = MEMORY_ALLOC ( MEMID_HCD_4k, FALSE );

	if ( ! slot->Output )
	{
		goto failed;
	}

	slot->Output_Physical = ( (struct Mem_FreeNode *)slot->Output )->mfn_Addr;
	MEM_SET ( slot->Input, 0, 4096 );
	MEM_SET ( slot->Output, 0, 4096 );

	if ( ! XHCI_Command ( hn, 0, 0, 0, XHCI_TRB_TYPE ( XHCI_TRB_ENABLE_SLOT ), &slot_id ) )
	{
		goto failed;
	}

	if ( ( ! slot_id ) || ( slot_id > state->Max_Slots ) )
	{
		goto failed;
	}

	slot->Slot_ID = slot_id;
	slot->Function = fn;
	slot->Root_Port = port;
	slot->Context_Entries = 1;
	slot->EP0_Packet = packet;
	state->Slots[slot_id] = slot;
	dcbaa = state->DCBAA;
	dcbaa[slot_id * 2] = LE_SWAP32 ( slot->Output_Physical );
	input = slot->Input;
	output = slot->Output;

	/* Input Control Context: add Slot and EP0 contexts. */
	input[1] = LE_SWAP32 ( 3 );
	input[state->Context_Size / 4] = LE_SWAP32 ( route | ( speed << 20 ) | ( 1 << 27 ) );
	input[state->Context_Size / 4 + 1] = LE_SWAP32 ( port << 16 );
	input[state->Context_Size / 4 + 2] = LE_SWAP32 ( tt_slot | ( tt_port << 8 ) );
	input[state->Context_Size / 2 + 1] = LE_SWAP32 ( ( packet << 16 ) | ( 4 << 3 ) | ( 3 << 1 ) );
	input[state->Context_Size / 2 + 2] = LE_SWAP32 ( slot->EndPoints[1].Physical | 1 );
	input[state->Context_Size / 2 + 4] = LE_SWAP32 ( 8 );

	if ( ! XHCI_Command ( hn, slot->Input_Physical, 0, 0,
		XHCI_TRB_TYPE ( XHCI_TRB_ADDRESS_DEVICE ) | ( slot_id << 24 ), NULL ) )
	{
		goto failed;
	}

	slot->Address = LE_SWAP32 ( output[3] ) & 0xff;

	return ( slot );

failed:
	if ( slot->Slot_ID )
	{
		XHCI_Command ( hn, 0, 0, 0,
			XHCI_TRB_TYPE ( XHCI_TRB_DISABLE_SLOT ) | ( slot->Slot_ID << 24 ), NULL );
		dcbaa = state->DCBAA;
		dcbaa[slot->Slot_ID * 2] = 0;
	}

	XHCI_Slot_Free ( hn, slot );

	return ( NULL );
}

// --
