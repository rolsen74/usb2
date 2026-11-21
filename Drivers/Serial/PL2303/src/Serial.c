/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE U32 Serial_Entry( struct USB2DriverIFace *Self, struct USB2_DriverMessage *msg )
{
struct SerialDriverBase *driver;
struct SerialContext *ctx;
struct SerialUnit *unit;

	if ( ( ! msg ) || ( ! msg->IUSB2 ) || ( ! msg->Interface ) || ( ! msg->InterfaceDescriptor ) )
	{
		return ( FALSE );
	}

	if ( ! ( ( ( msg->VendorID == SERIAL_PL2303_VENDOR ) && ( msg->DeviceID == SERIAL_PL2303_PRODUCT ) ) ||
		( ( msg->InterfaceDescriptor->InterfaceClass == USBCLASS_CDC_Control ) &&
		  ( msg->InterfaceDescriptor->InterfaceSubClass == SERIAL_ACM_SUBCLASS ) ) ) )
	{
		return ( FALSE );
	}

	driver = (PTR) Self->Data.LibBase;
	ctx = & driver->Context;
	ctx->IUSB2 = msg->IUSB2;
	IUSB2 = msg->IUSB2;

	unit = Serial_Alloc( ctx, msg );

	if ( ! unit )
	{
		return ( FALSE );
	}

	Serial_Main( ctx, unit );
	Serial_Free( ctx, unit );

	return ( TRUE );
}

// --
