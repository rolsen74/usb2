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
XHCI_Endpoint_Update0 ( struct USB2_HCDNode * hn, struct XHCI_Slot * slot, U32 packet )
{
	struct USBBase * usbbase;
	struct XHCI_State * state;

	U32 * input;
	U32 * endpoint;
	U32 value;

	usbbase = hn->hn_USBBase;
	state = hn->hn_HCD.XHCI.State;

	if ( ( packet != 8 ) && ( packet != 16 ) && ( packet != 32 ) && ( packet != 64 ) )
	{
		return ( FALSE );
	}

	MEM_SET ( slot->Input, 0, 4096 );
	MEM_COPY ( (U8 *)slot->Output + state->Context_Size,
		(U8 *)slot->Input + state->Context_Size * 2, state->Context_Size );
	input = slot->Input;
	input[1] = LE_SWAP32 ( 2 );
	endpoint = (U32 *)( (U8 *)slot->Input + state->Context_Size * 2 );
	value = LE_SWAP32 ( endpoint[1] );
	value &= 0x0000ffffUL;
	value |= packet << 16;
	endpoint[1] = LE_SWAP32 ( value );

	if ( ! XHCI_Command ( hn, slot->Input_Physical, 0, 0,
		XHCI_TRB_TYPE ( XHCI_TRB_EVALUATE_CTX ) | ( slot->Slot_ID << 24 ), NULL ) )
	{
		return ( FALSE );
	}

	slot->EP0_Packet = packet;

	return ( TRUE );
}

// --
