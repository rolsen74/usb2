
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
XHCI_Port_Protocol_Parse ( struct USB2_HCDNode * hn, U32 offset, U32 header, U32 limit )
{
	struct XHCI_State * state;
	struct USBBase * usbbase;
	struct XHCI_PortProtocol protocol;

	U64 rate;
	U32 info;
	U32 count;
	U32 first;
	U32 ports;
	U32 index;
	U32 value;
	U32 id;
	U32 speed;
	U32 exponent;
	U32 next;
	U32 minor;

	state = hn->hn_HCD.XHCI.State;
	usbbase = hn->hn_USBBase;
	next = ( ( header >> 8 ) & 255 ) * 4;

	if ( ( (U64) offset + 16 > limit ) || ( next && next < 16 ) )
	{
		return ( FALSE );
	}

	MEM_SET ( &protocol, 0, sizeof ( protocol ) );
	protocol.Major = header >> 24;
	minor = ( header >> 16 ) & 255;
	info = PCI_READLONG ( offset + 8 );
	first = info & 255;
	ports = ( info >> 8 ) & 255;
	count = info >> 28;

	if ( ( ( protocol.Major != 2 ) && ( protocol.Major != 3 ) ) ||
		( ! first ) || ( ! ports ) || ( first + ports - 1 > state->Max_Ports ) ||
		( (U64) offset + 16 + count * 4 > limit ) || ( next && next < 16 + count * 4 ) )
	{
		return ( FALSE );
	}

	protocol.Slot_Type = PCI_READLONG ( offset + 12 ) & 31;

	if ( ! count )
	{
		if ( protocol.Major == 2 )
		{
			protocol.Speed[1] = USBSPEED_Full + 1;
			protocol.Speed[2] = USBSPEED_Low + 1;
			protocol.Speed[3] = USBSPEED_High + 1;
		}
		else
		{
			protocol.Speed[4] = USBSPEED_Super + 1;

			if ( minor >= 0x10 )
			{
				protocol.Speed[5] = USBSPEED_Super + 1;
			}

			if ( minor >= 0x20 )
			{
				protocol.Speed[6] = USBSPEED_Super + 1;
				protocol.Speed[7] = USBSPEED_Super + 1;
			}
		}
	}

	for ( index = 0; index < count; index++ )
	{
		value = PCI_READLONG ( offset + 16 + index * 4 );
		id = value & 15;
		rate = value >> 16;
		exponent = ( value >> 4 ) & 3;

		while ( exponent )
		{
			rate *= 1000;
			exponent--;
		}

		if ( ( ! id ) || ( ! rate ) )
		{
			continue;
		}

		if ( protocol.Major == 3 )
		{
			speed = USBSPEED_Super;
		}
		else if ( rate == 1500000 )
		{
			speed = USBSPEED_Low;
		}
		else if ( rate == 12000000 )
		{
			speed = USBSPEED_Full;
		}
		else if ( rate == 480000000 )
		{
			speed = USBSPEED_High;
		}
		else
		{
			continue;
		}

		protocol.Speed[id] = speed + 1;

		// In the absence of extended hub speed status, prefer the 5 Gbit/s ID.
		if ( ( ! protocol.Speed_ID[speed] ) ||
			( ( speed == USBSPEED_Super ) && ( rate == 5000000000ULL ) ) )
		{
			protocol.Speed_ID[speed] = id;
		}
	}

	for ( id = 1; id < 16; id++ )
	{
		if ( ( protocol.Speed[id] ) && ( ! protocol.Speed_ID[protocol.Speed[id] - 1] ) )
		{
			protocol.Speed_ID[protocol.Speed[id] - 1] = id;
		}
	}

	for ( index = first; index < first + ports; index++ )
	{
		if ( state->Port[index].Major )
		{
			return ( FALSE );
		}

		state->Port[index] = protocol;
	}

	return ( TRUE );
}

// --
