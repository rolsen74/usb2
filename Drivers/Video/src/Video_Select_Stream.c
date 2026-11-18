
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE S32
Video_Select_Stream ( struct VideoUnit *unit, U32 payloadsize )
{
	struct USB2_Config_Desc *config;

	const U8 *desc;
	U32 remaining;
	U32 length;
	U32 packet;
	U32 extra;
	U32 capacity;
	U32 best;
	const U8 *companion;
	U8 alt;
	U8 active;

	config = (PTR) unit->StartMsg->ConfigDescriptors;
	desc = (PTR) config;
	remaining = LE_SWAP16( config->TotalLength );
	best = 0;
	alt = 0;
	active = FALSE;

	while ( remaining >= 2 )
	{
		length = desc[0];

		if ( ( length < 2 ) || ( length > remaining ) )
		{
			return ( FALSE );
		}

		if ( ( desc[1] == DSCTYPE_Interface ) && ( length >= sizeof( struct USB2_Interface_Desc ) ) )
		{
			alt = desc[3];
			active = ( desc[2] == unit->InterfaceNr ) && ( alt ) && ( desc[5] == 0x0e ) && ( desc[6] == 0x02 );
		}
		else if ( ( active ) && ( desc[1] == DSCTYPE_EndPoint ) &&
			( length >= sizeof( struct USB2_EndPoint_Desc ) ) &&
			( desc[2] & EPADR_Dir_Mask ) && ( desc[2] & 0x0f ) && ( desc[6] ) &&
			( ( desc[3] & EPATT_Type_Mask ) == EPATT_Type_Isochronous ) )
		{
			packet = desc[4] | ( (U32) desc[5] << 8 );
			extra = ( packet >> 11 ) & 3;
			packet &= 0x7ff;
			capacity = packet * ( extra + 1 );

			// The stack does not program SuperSpeed companion burst or Mult.
			companion = desc + length;

			if ( ( remaining - length >= 6 ) && ( companion[0] >= 6 ) && ( companion[1] == 0x30 ) &&
				( ( companion[2] ) || ( companion[3] & 3 ) ) )
			{
				capacity = 0;
			}

			if ( ( packet <= 1024 ) && ( extra <= 2 ) && ( capacity >= payloadsize ) &&
				( ( ! best ) || ( capacity < best ) ) )
			{
				best = capacity;
				unit->StreamAlt = alt;
				unit->EndPoint = desc[2];
			}
		}

		desc += length;
		remaining -= length;
	}

	unit->StreamPacketSize = best;

	return ( best != 0 );
}

// --
