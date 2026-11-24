
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Firmware.h"
#include "Radio.h"

// --

U32
RTL8188EU_CountTXEndpoints( struct EthernetUnit *unit )
{
	U8 *config;
	U8 *desc;
	U32 size;
	U32 offset;
	U32 count;
	U32 length;
	USB2_ID function;
	S32 selected;

	function = unit->unit_USB_Interface->FKT_ID;
	count = 0;
	selected = FALSE;

	if ( ( USB2_Attr_Get( function, USB2Tag_Attr_ConfigSize, & size, sizeof( size ) ) != sizeof( size ) ) ||
		( size < 9 ) || ( size > 65535 ) )
	{
		return ( 0 );
	}

	config = AllocVecTags( size, TAG_END );

	if ( ! config )
	{
		return ( 0 );
	}

	if ( USB2_Attr_Get( function, USB2Tag_Attr_ConfigDesc, config, size ) != size )
	{
		FreeVec( config );

		return ( 0 );
	}

	for( offset = 0; size - offset >= 2; offset += length )
	{
		desc = config + offset;
		length = desc[0];

		if ( ( length < 2 ) || ( length > size - offset ) )
		{
			count = 0;

			break;
		}

		if ( desc[1] == DSCTYPE_Interface )
		{
			if ( selected )
			{
				break;
			}

			selected = ( length >= 9 ) && ( desc[2] == unit->unit_USB_Interface->IFC_Num ) && ( ! desc[3] );
		}
		else if ( ( selected ) && ( desc[1] == DSCTYPE_EndPoint ) && ( length >= 7 ) &&
			( ( desc[2] & EPADR_Dir_Mask ) == EPADR_Dir_Out ) && ( ( desc[3] & 3 ) == EPATT_Type_Bulk ) )
		{
			count++;
		}
	}

	FreeVec( config );

	return ( count );
}

// --
