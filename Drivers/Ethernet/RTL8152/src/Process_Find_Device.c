
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

struct USB_Ethernet_Device
{
	STR		name;
	U32				vendor;
	U32				device;
};

static const struct USB_Ethernet_Device USB_Device_IDs[] =
{
	{ "Realtek 8152",	0x0bda, 0x8152 },
	{ NULL,				0x0000,	0x0000 }
};

// --

int
unit_Process_FindUSBDevice( struct devUnit *unit )
{
	struct USB2_Interface *ifc;
	struct USB2IFace *IUSB2;

	U32 index;

	IUSB2 = unit->unit_IUSB2;

	for( index = 0; USB_Device_IDs[index].name; index++ )
	{
		unit->unit_USBInterface = IUSB2->USB2_Ifc_FindTags(
			USB2Tag_Find_VendorID, USB_Device_IDs[index].vendor,
			USB2Tag_Find_DeviceID, USB_Device_IDs[index].device,
			USB2Tag_Find_Class, USBCLASS_Vendor_Specific,
			TAG_END );

		if ( unit->unit_USBInterface )
		{
			return ( TRUE );
		}

		ifc = IUSB2->USB2_Ifc_FindTags(
			USB2Tag_Find_VendorID, USB_Device_IDs[index].vendor,
			USB2Tag_Find_DeviceID, USB_Device_IDs[index].device,
			TAG_END );

		if ( ifc )
		{
			IExec->DebugPrintF( "RTL8152: 0BDA:8152 has an active USB interface %lu, but no vendor-specific match\n",
				ifc->IFC_Num );
		}
	}

	return ( FALSE );
}

// --
