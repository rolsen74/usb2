
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

	S32 stat;

	if ( ( ! msg ) || ( ! msg->IUSB2 ) || ( ! msg->Interface ) || ( ! msg->InterfaceDescriptor ) )
	{
		return ( FALSE );
	}

	// PTP and MTP cameras use the Still Image Capture interface.
	if ( ( msg->InterfaceDescriptor->InterfaceSubClass != 1 ) ||
		( msg->InterfaceDescriptor->InterfaceProtocol != 1 ) )
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

	return ( stat );
}

// --
