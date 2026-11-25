/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE static S32
MatchAlias( const char *alias, const char *expected )
{
	while( ( *alias ) && ( *alias == *expected ) )
	{
		alias++;
		expected++;
	}

	return ( *alias == *expected );
}

// --

SEC_CODE static S32
Serial_Binding( const struct USB2_DriverMessage *msg )
{
	const struct Library *usb2;

	const char *alias;
	S32 acm;

	usb2 = msg->IUSB2->Data.LibBase;
	alias = NULL;
	acm = ( ( msg->InterfaceDescriptor->InterfaceClass == USBCLASS_CDC_Control ) &&
		( msg->InterfaceDescriptor->InterfaceSubClass == SERIAL_ACM_SUBCLASS ) );

	// The class alias is available from usb2.device 1.64.
	if ( ( usb2 ) && ( ( usb2->lib_Version > 1 ) ||
		( ( usb2->lib_Version == 1 ) && ( usb2->lib_Revision >= 64 ) ) ) )
	{
		alias = msg->ClassAlias;
	}

	if ( ( alias ) && ( *alias ) )
	{
		if ( MatchAlias( alias, "pl2303" ) )
		{
			return ( FALSE );
		}

		return ( ( ( MatchAlias( alias, "cdc-acm" ) ) && ( acm ) ) ? TRUE : -1 );
	}

	// Preserve binding for the original class files.
	if ( ( msg->VendorID == SERIAL_PL2303_VENDOR ) && ( msg->DeviceID == SERIAL_PL2303_PRODUCT ) )
	{
		return ( FALSE );
	}

	return ( acm ? TRUE : -1 );
}

// --

SEC_CODE U32 Serial_Entry( struct USB2DriverIFace *Self, struct USB2_DriverMessage *msg )
{
struct SerialDriverBase *driver;
struct SerialContext *ctx;
struct SerialUnit *unit;

S32 acm;

	if ( ( ! msg ) || ( ! msg->IUSB2 ) || ( ! msg->Interface ) || ( ! msg->InterfaceDescriptor ) )
	{
		return ( FALSE );
	}

	acm = Serial_Binding( msg );

	if ( acm < 0 )
	{
		return ( FALSE );
	}

	driver = (PTR) Self->Data.LibBase;
	ctx = & driver->Context;
	ctx->IUSB2 = msg->IUSB2;
	IUSB2 = msg->IUSB2;

	unit = Serial_Alloc( ctx, msg, acm );

	if ( ! unit )
	{
		return ( FALSE );
	}

	Serial_Main( ctx, unit );
	Serial_Free( ctx, unit );

	return ( TRUE );
}

// --
