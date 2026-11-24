
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
Video_Find_Endpoint( struct VideoUnit *unit )
{
	const struct USB2_Config_Desc *config;

	const U8 *desc;
	U32 remaining;
	U32 length;
	U8 endpoint;
	U8 bulk;
	U8 active;

	config = (const struct USB2_Config_Desc *) unit->StartMsg->ConfigDescriptors;

	if ( ( ! config ) || ( config->Length < sizeof( *config ) ) ||
		( config->DescriptorType != DSCTYPE_Configuration ) )
	{
		return ( FALSE );
	}

	desc = (const U8 *) config;
	remaining = LE_SWAP16( config->TotalLength );
	active = FALSE;
	endpoint = 0;
	bulk = 0;

	while ( remaining >= 2 )
	{
		length = desc[0];

		if ( ( length < 2 ) || ( length > remaining ) )
		{
			return ( FALSE );
		}

		if ( ( desc[1] == DSCTYPE_Interface ) && ( length >= sizeof( struct USB2_Interface_Desc ) ) )
		{
			active = ( desc[2] == unit->InterfaceNr ) && ( ! desc[3] ) &&
				( desc[5] == 0x0e ) && ( desc[6] == 0x02 );
		}
		else if ( ( active ) && ( desc[1] == 0x24 ) && ( length >= 13 ) && ( desc[2] == 1 ) )
		{
			if ( endpoint )
			{
				return ( FALSE );
			}

			endpoint = desc[6];

			if ( ( ! ( endpoint & EPADR_Dir_Mask ) ) || ( endpoint & 0x70 ) || ( ! ( endpoint & 0x0f ) ) )
			{
				return ( FALSE );
			}
		}
		else if ( ( active ) && ( endpoint ) && ( desc[1] == DSCTYPE_EndPoint ) &&
			( length >= sizeof( struct USB2_EndPoint_Desc ) ) && ( desc[2] == endpoint ) &&
			( ( desc[3] & EPATT_Type_Mask ) == EPATT_Type_Bulk ) )
		{
			bulk = endpoint;
		}

		desc += length;
		remaining -= length;
	}

	unit->VideoEndPoint = endpoint;
	unit->BulkEndPoint = bulk;

	return ( endpoint != 0 );
}

// --
