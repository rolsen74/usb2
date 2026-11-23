
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
XHCI_Slots_Retire ( struct USB2_HCDNode * hn )
{
	struct RealFunctionNode * fn;
	struct RealRequest * req;
	struct XHCI_State * state;
	struct XHCI_Slot * slot;
	struct USB2_Node * node;
	struct USBBase * usbbase;

	U8 keep[sizeof ( state->Slots ) / sizeof ( state->Slots[0] )];
	U32 index;
	U32 child;
	S32 retired;

	usbbase = hn->hn_USBBase;
	state = hn->hn_HCD.XHCI.State;

	if ( ( ! state ) || ( ! state->DCBAA ) )
	{
		return ( TRUE );
	}

	MEM_SET ( keep, 0, sizeof ( keep ) );

	SEMAPHORE_OBTAIN ( &usbbase->usb_Fkt_Semaphore );

	SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

	for ( node = usbbase->usb_Fkt_Header.uh_Head; node; node = Node_Next ( node ) )
	{
		fn = node->un_Data;

		if ( ( fn->fkt_HCD != hn ) || ( fn->fkt_Detach ) || ( fn->fkt_FreeMe ) )
		{
			continue;
		}

		slot = XHCI_Slot_Find ( hn, fn->fkt_NotifyID );

		if ( slot )
		{
			keep[slot->Slot_ID] = TRUE;
		}
	}

	SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

	SEMAPHORE_RELEASE ( &usbbase->usb_Fkt_Semaphore );

	for ( req = hn->hn_Active_Transfer_List.uh_Head; req; req = Node_Next ( req ) )
	{
		slot = req->req_HCD.XHCI.Slot;

		if ( slot )
		{
			keep[slot->Slot_ID] = TRUE;
		}
	}

	do
	{
		retired = FALSE;

		for ( index = 1; index <= state->Max_Slots; index++ )
		{
			slot = state->Slots[index];

			if ( ( ! slot ) || ( keep[index] ) )
			{
				continue;
			}

			// A hub slot must outlive its downstream slots.
			for ( child = 1; child <= state->Max_Slots; child++ )
			{
				if ( ( state->Slots[child] ) && ( state->Slots[child]->Parent_ID == slot->Function_ID ) )
				{
					break;
				}
			}

			if ( child <= state->Max_Slots )
			{
				continue;
			}

			if ( ! XHCI_Slot_Disable ( hn, slot ) )
			{
				return ( FALSE );
			}

			retired = TRUE;
		}
	} while ( retired );

	return ( TRUE );
}

// --
