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
XHCI_Chip_Init ( struct USB2_HCDNode * hn )
{
	struct USBBase * usbbase;
	struct XHCI_State * state;

	U32 * dcbaa;
	U32 * scratch;
	U32 * erst;
	U32 index;
	U32 physical;

	usbbase = hn->hn_USBBase;
	state = hn->hn_HCD.XHCI.State;

	if ( ( ! XHCI_Ring_Alloc ( hn, &state->Commands ) ) ||
		( ! XHCI_Ring_Alloc ( hn, &state->Events ) ) )
	{
		goto failed;
	}

	state->DCBAA = MEMORY_ALLOC ( MEMID_HCD_4k, FALSE );

	if ( ! state->DCBAA )
	{
		goto failed;
	}

	state->DCBAA_Physical = ( (struct Mem_FreeNode *)state->DCBAA )->mfn_Addr;
	state->ERST = MEMORY_ALLOC ( MEMID_HCD_4k, FALSE );

	if ( ! state->ERST )
	{
		goto failed;
	}

	state->ERST_Physical = ( (struct Mem_FreeNode *)state->ERST )->mfn_Addr;
	MEM_SET ( state->DCBAA, 0, 4096 );
	MEM_SET ( state->ERST, 0, 4096 );
	dcbaa = state->DCBAA;
	erst = state->ERST;

	if ( state->Scratchpad_Count )
	{
		if ( state->Scratchpad_Count > 1023 )
		{
			goto failed;
		}

		state->Scratchpad_MemID = ( state->Scratchpad_Count > 512 ) ? MEMID_HCD_8k : MEMID_HCD_4k;
		state->Scratchpad_Array = MEMORY_ALLOC ( state->Scratchpad_MemID, FALSE );

		if ( ! state->Scratchpad_Array )
		{
			goto failed;
		}

		state->Scratchpad_Physical = ( (struct Mem_FreeNode *)state->Scratchpad_Array )->mfn_Addr;
		state->Scratchpads = MEM_ALLOCVEC ( state->Scratchpad_Count * sizeof ( PTR ), TRUE );

		if ( ! state->Scratchpads )
		{
			goto failed;
		}
		MEM_SET ( state->Scratchpad_Array, 0, ( state->Scratchpad_MemID == MEMID_HCD_8k ) ? 8192 : 4096 );
		scratch = state->Scratchpad_Array;

		for ( index = 0; index < state->Scratchpad_Count; index++ )
		{
			state->Scratchpads[index] = MEMORY_ALLOC ( MEMID_HCD_4k, FALSE );

			if ( ! state->Scratchpads[index] )
			{
				goto failed;
			}

			physical = ( (struct Mem_FreeNode *)state->Scratchpads[index] )->mfn_Addr;
			MEM_SET ( state->Scratchpads[index], 0, 4096 );
			scratch[index * 2] = LE_SWAP32 ( physical );
		}

		dcbaa[0] = LE_SWAP32 ( state->Scratchpad_Physical );
	}

	erst[0] = LE_SWAP32 ( state->Events.Physical );
	erst[2] = LE_SWAP32 ( XHCI_RING_TRBS );

	PCI_WRITELONG ( state->Operational + XHCI_CONFIG, state->Max_Slots );
	PCI_WRITELONG ( state->Operational + XHCI_DCBAAP, state->DCBAA_Physical );
	PCI_WRITELONG ( state->Operational + XHCI_DCBAAP + 4, 0 );
	PCI_WRITELONG ( state->Operational + XHCI_CRCR, state->Commands.Physical | 1 );
	PCI_WRITELONG ( state->Operational + XHCI_CRCR + 4, 0 );
	PCI_WRITELONG ( state->Runtime + XHCI_ERSTSZ, 1 );
	PCI_WRITELONG ( state->Runtime + XHCI_ERDP, state->Events.Physical );
	PCI_WRITELONG ( state->Runtime + XHCI_ERDP + 4, 0 );

	XHCI_Sync ();

	PCI_WRITELONG ( state->Runtime + XHCI_ERSTBA, state->ERST_Physical );
	PCI_WRITELONG ( state->Runtime + XHCI_ERSTBA + 4, 0 );
	PCI_WRITELONG ( state->Runtime + XHCI_IMOD, 0 );

	return ( TRUE );

failed:
	XHCI_Chip_Deinit ( hn );

	return ( FALSE );
}

// --
