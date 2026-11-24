
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

U32
_usb_Entry( struct USB2DriverIFace *Self UNUSED, struct USB2_DriverMessage *msg )
{
	struct USB_Struct us;
	struct USB2_Interface_Desc *descriptor;

	S32 stat;

	if ( ( ! msg ) || ( ! msg->IUSB2 ) || ( ! msg->Interface ) || ( ! msg->InterfaceDescriptor ) )
	{
		return ( FALSE );
	}

	descriptor = msg->InterfaceDescriptor;

	// Android can expose MTP on a vendor-specific interface.
	if ( ! ( ( ( descriptor->InterfaceClass == USBCLASS_Image ) &&
		( descriptor->InterfaceSubClass == 1 ) && ( descriptor->InterfaceProtocol == 1 ) ) ||
		( ( descriptor->InterfaceClass == 0xff ) && ( descriptor->InterfaceSubClass == 0xff ) &&
		  ( ! descriptor->InterfaceProtocol ) && ( descriptor->InterfaceStrNr ) &&
		  ( descriptor->NumEndPoints == 3 ) ) ) )
	{
		return ( FALSE );
	}

	if ( ! _fs_Acquire() )
	{
		return ( FALSE );
	}

	memset( & us, 0, sizeof( us ) );

	us.us_StartMsg = msg;

	stat = _usb_Init( & us );

	if ( stat )
	{
		_usb_Loop( & us );
	}

	_usb_Free( & us );

	_fs_Release();

	return ( stat && us.us_Published );
}

// --
