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
XHCI_Chip_Preinit ( struct USB2_HCDNode * hn )
{
	struct USBBase * usbbase;
	struct XHCI_State * state;

	U32 cap;
	U32 hcs1;
	U32 hcs2;
	U32 hcc1;
	U32 ext;
	U32 value;
	U32 count;
	U32 wait;
	U32 limit;
	U32 protocols;

	usbbase = hn->hn_USBBase;
	if ( ( ! hn->hn_PCIDevResource ) || ( hn->hn_PCIDevResource->Size < 0x20 ) )
	{
		return ( FALSE );
	}

	limit = hn->hn_PCIDevResource->Size;

	state = MEM_ALLOCVEC ( sizeof ( *state ), TRUE );

	if ( ! state )
	{
		return ( FALSE );
	}

	hn->hn_HCD.XHCI.State = state;
	cap = PCI_READLONG ( XHCI_CAPLENGTH );
	hcs1 = PCI_READLONG ( XHCI_HCSPARAMS1 );
	hcs2 = PCI_READLONG ( XHCI_HCSPARAMS2 );
	hcc1 = PCI_READLONG ( XHCI_HCCPARAMS1 );
	state->Capability_Length = cap & 0xff;
	state->Operational = state->Capability_Length;
	state->Runtime = PCI_READLONG ( XHCI_RTSOFF ) & ~0x1fUL;
	state->Doorbells = PCI_READLONG ( XHCI_DBOFF ) & ~0x3UL;
	state->Max_Slots = hcs1 & 0xff;
	state->Max_Ports = ( hcs1 >> 24 ) & 0xff;
	state->Context_Size = ( hcc1 & 4 ) ? 64 : 32;
	state->Scratchpad_Count = ( ( hcs2 >> 27 ) & 0x1f ) | ( ( hcs2 >> 16 ) & 0x3e0 );

	protocols = 0;

	if ( ( state->Capability_Length < 0x20 ) || ( state->Max_Slots == 0 ) ||
		( state->Max_Ports == 0 ) ||
		( XHCI_PORTSC ( state->Max_Ports ) + state->Operational + 4 > limit ) ||
		( (U64) state->Runtime + XHCI_ERDP + 8 > limit ) ||
		( (U64) state->Doorbells + ( state->Max_Slots + 1 ) * 4 > limit ) || ( ! ( PCI_READLONG ( state->Operational + XHCI_PAGESIZE ) & 1 ) ) )
	{
		USBERROR ( "XHCI: unsupported controller capability registers" );
		MEM_FREEVEC ( state );
		hn->hn_HCD.XHCI.State = NULL;

		return ( FALSE );
	}

	ext = ( hcc1 >> 16 ) * 4;

	for ( count = 0; ( ext ) && ( count < 64 ); count++ )
	{
		if ( (U64) ext + 4 > limit )
		{
			goto failed;
		}

		value = PCI_READLONG ( ext );

		if ( ( value & 0xff ) == 2 )
		{
			if ( ! XHCI_Port_Protocol_Parse ( hn, ext, value, limit ) )
			{
				goto failed;
			}

			protocols++;
		}
		else if ( ( value & 0xff ) == 1 )
		{
			if ( ( (U64) ext + 8 > limit ) || ( state->Legacy_Claimed ) )
			{
				goto failed;
			}

			state->Legacy_Offset = ext;
			PCI_WRITEBYTE ( ext + 3, 1 );
			state->Legacy_Claimed = TRUE;

			for ( wait = 0; wait < 1000; wait++ )
			{
				if ( ! ( PCI_READLONG ( ext ) & 0x00010000UL ) )
				{
					break;
				}

				HCD_WAIT_MS ( hn, 1 );
			}

			if ( wait == 1000 )
			{
				USBERROR ( "XHCI: firmware did not release controller ownership" );
				goto failed;
			}

			PCI_WRITELONG ( ext + 4, 0 );
		}

		ext += ( ( value >> 8 ) & 0xff ) * 4;

		if ( ! ( value & 0x0000ff00UL ) )
		{
			break;
		}
	}

	if ( ( ! protocols ) || ( count == 64 ) )
	{
		goto failed;
	}

	hn->hn_HUB_NumPorts = state->Max_Ports;

	return ( TRUE );

failed:
	USBERROR ( "XHCI: could not initialize extended capabilities" );

	if ( state->Legacy_Claimed )
	{
		PCI_WRITEBYTE ( state->Legacy_Offset + 3, 0 );
	}

	MEM_FREEVEC ( state );
	hn->hn_HCD.XHCI.State = NULL;

	return ( FALSE );
}

// --
