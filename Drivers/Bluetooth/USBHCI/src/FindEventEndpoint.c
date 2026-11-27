
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

U8
BTUSB_FindEventEndpoint( struct USB2_DriverMessage *message, U8 *recipient )
{
	struct USB2_Device_Desc device;
	struct USB2IFace *usb2;

	const U8 *descriptor;
	U32 size;
	U32 length;
	U32 interface;
	U8 endpoint;
	S32 active;

	usb2 = message->IUSB2;
	interface = message->Interface->IFC_Num;
	descriptor = (PTR) message->ConfigDescriptors;
	endpoint = 0;
	active = FALSE;
	*recipient = interface ? REQTYPE_Interface : REQTYPE_Device;

	if ( ( ! descriptor ) ||
		( usb2->USB2_Attr_Get( message->Interface->FKT_ID, USB2Tag_Attr_ConfigSize, & size, sizeof( size ) ) != sizeof( size ) ) )
	{
		return ( 0 );
	}

	if ( usb2->USB2_Attr_Get( message->Interface->FKT_ID, USB2Tag_Attr_DeviceDesc, & device, sizeof( device ) ) == sizeof( device ) )
	{
		*recipient = ( ( device.DeviceClass == USBCLASS_Wireless_Controller ) &&
			( device.DeviceSubClass == 1 ) && ( device.DeviceProtocol == 1 ) ) ? REQTYPE_Device : REQTYPE_Interface;
	}

	while( size >= 2 )
	{
		length = descriptor[0];

		if ( ( length < 2 ) || ( length > size ) )
		{
			return ( 0 );
		}

		if ( ( descriptor[1] == DSCTYPE_Interface_Association ) && ( length >= 8 ) &&
			( interface >= descriptor[2] ) && ( interface - descriptor[2] < descriptor[3] ) )
		{
			*recipient = REQTYPE_Interface;
		}
		else if ( descriptor[1] == DSCTYPE_Interface )
		{
			active = ( length >= 9 ) && ( descriptor[2] == interface ) &&
				( descriptor[3] == 0 ) && ( descriptor[5] == USBCLASS_Wireless_Controller ) &&
				( descriptor[6] == 1 ) && ( descriptor[7] == 1 );
		}
		else if ( ( active ) && ( ! endpoint ) && ( descriptor[1] == DSCTYPE_EndPoint ) &&
			( length >= 7 ) && ( descriptor[2] & EPADR_Dir_In ) &&
			( EPADR_Dir_Number( descriptor[2] ) ) &&
			( ( descriptor[3] & EPATT_Type_Mask ) == EPATT_Type_Interrupt ) )
		{
			endpoint = descriptor[2];
		}

		descriptor += length;
		size -= length;
	}

	// This legacy dongle used device-recipient HCI commands on interface zero.
	if ( ( message->VendorID == 0x1131 ) && ( message->DeviceID == 0x1001 ) && ( ! interface ) )
	{
		*recipient = REQTYPE_Device;
	}

	return ( size ? 0 : endpoint );
}

// --
