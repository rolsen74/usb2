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

	driver = (PTR) Self->Data.LibBase;
	ctx = & driver->Context;
	ctx->IUSB2 = msg->IUSB2;
	IUSB2 = msg->IUSB2;

	if ( ( msg->VendorID != 0x067b ) || ( msg->DeviceID != 0x2303 ) )
	{
		return ( FALSE );
	}

	if ( ! Serial_CreateDevice( ctx ) )
	{
		return ( FALSE );
	}

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
