
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
	U32 stride;
	U32 speed;
	const U8 *companion;
	U8 alt;
	U8 active;

	config = (PTR) unit->StartMsg->ConfigDescriptors;
	desc = (PTR) config;
	remaining = LE_SWAP16( config->TotalLength );
	best = 0;
	stride = 0;
	speed = 0;

	USB2_Attr_Get( unit->StartMsg->Function->FKT_ID, USB2Tag_Attr_Speed, & speed, sizeof( speed ) );

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
			( desc[2] == unit->VideoEndPoint ) && ( desc[6] ) && ( desc[6] <= 16 ) &&
			( ( desc[3] & EPATT_Type_Mask ) == EPATT_Type_Isochronous ) )
		{
			packet = desc[4] | ( (U32) desc[5] << 8 );
			extra = ( packet >> 11 ) & 3;
			packet &= 0x7ff;
			capacity = packet * ( extra + 1 );

			if ( speed == USBSPEED_Super )
			{
				companion = desc + length;
				capacity = 0;

				if ( ( ! extra ) && ( remaining - length >= 6 ) &&
					( companion[0] == 6 ) && ( companion[1] == 0x30 ) &&
					( ! companion[2] ) && ( ! companion[3] ) )
				{
					capacity = Video_LE16( & companion[4] );

					if ( capacity > packet )
					{
						capacity = 0;
					}
				}
			}

			if ( ( packet <= 1024 ) && ( extra <= 2 ) && ( capacity >= payloadsize ) &&
				( ( ! best ) || ( capacity < best ) ) )
			{
				best = capacity;
				stride = packet * ( extra + 1 );
				unit->StreamAlt = alt;
				unit->EndPoint = desc[2];
				unit->StreamInterval = desc[6];
			}
		}

		desc += length;
		remaining -= length;
	}

	unit->StreamPacketSize = stride;

	return ( best != 0 );
}

// --
