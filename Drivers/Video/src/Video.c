/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE U32 Video_Entry( struct USB2DriverIFace *Self, struct USB2_DriverMessage *msg )
{
struct VideoDriverBase *driver;
struct VideoContext *ctx;
struct VideoUnit *unit;

	if ( ( ! msg ) || ( ! msg->InterfaceDescriptor ) )
	{
		return ( FALSE );
	}

	if ( ! ( ( ( msg->InterfaceDescriptor->InterfaceClass == USBCLASS_Video ) &&
		( msg->InterfaceDescriptor->InterfaceSubClass == 0x02 ) ) ||
		( ( msg->VendorID == 0x1415 ) && ( msg->DeviceID == 0x2000 ) &&
		( msg->InterfaceDescriptor->InterfaceClass == USBCLASS_Vendor_Specific ) &&
		( msg->InterfaceDescriptor->InterfaceNumber == 0 ) ) ) )
	{
		return ( FALSE );
	}

	driver = (PTR) Self->Data.LibBase;
	ctx = & driver->Context;
	ctx->IUSB2 = msg->IUSB2;
	IUSB2 = msg->IUSB2;

	if ( ! Video_CreateDevice( ctx ) )
	{
		return ( FALSE );
	}

	unit = Video_Alloc( ctx, msg );

	if ( ! unit )
	{
		return ( FALSE );
	}

	Video_Main( unit );
	Video_Free( unit );

	return ( TRUE );
}

// --
