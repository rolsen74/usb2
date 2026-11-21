
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <proto/exec.h>
#include "All.h"

// --

static U8
FindEventEndpoint( const U8 *descriptor, U32 size, U32 interface )
{
	U32 length;
	S32 active;

	active = FALSE;

	while( size >= 2 )
	{
		length = descriptor[0];

		if ( ( length < 2 ) || ( length > size ) )
		{
			break;
		}

		if ( descriptor[1] == DSCTYPE_Interface )
		{
			active = ( length >= 9 ) && ( descriptor[2] == interface ) &&
				( descriptor[3] == 0 ) && ( descriptor[5] == USBCLASS_Wireless_Controller ) &&
				( descriptor[6] == 1 ) && ( descriptor[7] == 1 );
		}
		else if ( ( active ) && ( descriptor[1] == DSCTYPE_EndPoint ) &&
			( length >= 7 ) && ( descriptor[2] & EPADR_Dir_In ) &&
			( EPADR_Dir_Number( descriptor[2] ) ) &&
			( ( descriptor[3] & EPATT_Type_Mask ) == EPATT_Type_Interrupt ) )
		{
			return ( descriptor[2] );
		}

		descriptor += length;
		size -= length;
	}

	return ( 0 );
}

// --

U8
BTUSB_FindEventEndpoint( struct USB2IFace *usb2, struct USB2_Interface *ifc )
{
	U8 *config;
	U8 endpoint;
	U32 size;

	if ( ( usb2->USB2_Attr_Get( ifc->FKT_ID, USB2Tag_Attr_ConfigSize, & size, sizeof( size ) ) != sizeof( size ) ) ||
		( size < 9 ) || ( size > 65535 ) )
	{
		return ( 0 );
	}

	config = AllocVecTags( size, TAG_END );

	if ( ! config )
	{
		return ( 0 );
	}

	endpoint = 0;

	if ( usb2->USB2_Attr_Get( ifc->FKT_ID, USB2Tag_Attr_ConfigDesc, config, size ) == size )
	{
		endpoint = FindEventEndpoint( config, size, ifc->IFC_Num );
	}

	FreeVec( config );

	return ( endpoint );
}

// --
