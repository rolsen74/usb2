
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

S32 Process_Find_Device( struct PrinterUnit *unit )
{
struct USB2_Interface *ifc;
S32 retval;

	USBERROR( "Printer : Process_Find_Device" );

	retval = FALSE;

	// USB ID is 16bit, so anything over 65535 is a unset value
	if (( unit->unit_USB_VendorID < 65536 )
	&&	( unit->unit_USB_DeviceID < 65536 ))
	{
		USBERROR( "Printer : __Find_DeviceID : Find DevID $%04lx:%04lx", unit->unit_USB_VendorID, unit->unit_USB_DeviceID );

		ifc = USB2_Ifc_FindTags(
			USB2Tag_Find_VendorID, unit->unit_USB_VendorID,
			USB2Tag_Find_DeviceID, unit->unit_USB_DeviceID,
	//		USB2Tag_Find_SeeClaimed, TRUE,
	//		USB2Tag_Find_Index
			TAG_END
		);
	}
	else
	{
		USBERROR( "Printer : __Find_Generic" );

		ifc = USB2_Ifc_FindTags(
			USB2Tag_Find_SubClass, USBPRT_SUBCLASS_Printer,
			USB2Tag_Find_Class, USBCLASS_Printer,
			USB2Tag_Find_SeeClaimed, TRUE,
	//		USB2Tag_Find_Index
			TAG_END
		);
	}

	if ( ! ifc )
	{
		USBERROR( "Printer : Failed to find Printer" );
		goto bailout;
	}

	unit->unit_USB_Interface = ifc;

	retval = TRUE;

bailout:

	return( retval );
}

// --
