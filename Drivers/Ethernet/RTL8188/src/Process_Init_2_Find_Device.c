
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

static const U16 RTL8188EU_Devices[][2] =
{
	{ 0x0bda, 0x8179 },
	{ 0x07b8, 0x8179 },
	{ 0x2357, 0x010c }
};

// --

S32
Process_Find_Device( struct EthernetUnit *unit )
{
	struct USB2_Interface *ifc;

	U32 index;

	for( index = 0; index < sizeof( RTL8188EU_Devices ) / sizeof( RTL8188EU_Devices[0] ); index++ )
	{
		ifc = USB2_Ifc_FindTags(
			USB2Tag_Find_VendorID, RTL8188EU_Devices[index][0],
			USB2Tag_Find_DeviceID, RTL8188EU_Devices[index][1],
			USB2Tag_Find_Class, USBCLASS_Vendor_Specific,
			TAG_END );

		if ( ( ifc ) && ( ifc->IFC_Num == 0 ) )
		{
			unit->unit_USB_Interface = ifc;

			return ( TRUE );
		}
	}

	USBERROR( "RTL8188EU: no supported USB adapter found" );

	return ( FALSE );
}

// --
