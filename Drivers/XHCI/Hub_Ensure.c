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
XHCI_Hub_Ensure ( struct USB2_HCDNode * hn, struct RealFunctionNode * fn )
{
	struct USBBase * usbbase;
	struct XHCI_State * state;
	struct XHCI_Slot * slot;

	U32 * input;
	U32 * context;
	U32 ports;
	U32 value;

	usbbase = hn->hn_USBBase;
	state = hn->hn_HCD.XHCI.State;
	slot = XHCI_Slot_Find ( hn, fn->fkt_NotifyID );

	if ( ( ! slot ) || ( ! fn->fkt_HUBDescriptor ) )
	{
		return ( FALSE );
	}

	if ( slot->Hub_Configured )
	{
		return ( TRUE );
	}

	ports = fn->fkt_HUBDescriptor->NbrPorts;

	if ( ! ports )
	{
		return ( FALSE );
	}

	MEM_SET ( slot->Input, 0, 4096 );
	MEM_COPY ( slot->Output, (U8 *)slot->Input + state->Context_Size, state->Context_Size );
	input = slot->Input;
	input[1] = LE_SWAP32 ( 1 );
	context = (U32 *)( (U8 *)slot->Input + state->Context_Size );
	value = LE_SWAP32 ( context[0] );
	value |= 1UL << 26;

	context[0] = LE_SWAP32 ( value );
	value = LE_SWAP32 ( context[1] );
	value &= 0x00ffffffUL;
	value |= ports << 24;
	context[1] = LE_SWAP32 ( value );
	value = LE_SWAP32 ( context[2] );
	value &= ~( 3UL << 16 );
	if ( fn->fkt_Speed == USBSPEED_High )
	{
		value |= ( ( LE_SWAP16 ( fn->fkt_HUBDescriptor->HubCharacteristics ) >> 5 ) & 3 ) << 16;
	}
	context[2] = LE_SWAP32 ( value );

	if ( ! XHCI_Command ( hn, slot->Input_Physical, 0, 0,
		XHCI_TRB_TYPE ( XHCI_TRB_CONFIGURE_EP ) | ( slot->Slot_ID << 24 ), NULL ) )
	{
		return ( FALSE );
	}

	slot->Hub_Configured = TRUE;

	return ( TRUE );
}

// --
