/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE U32 Audio_Entry( struct USB2DriverIFace *Self, struct USB2_DriverMessage *msg )
{
struct AudioDriverBase *driver;
struct AudioContext *ctx;
struct AudioUnit *unit;

	if ( ( ! msg ) || ( ! msg->Interface ) || ( ! msg->InterfaceDescriptor ) ||
		( msg->InterfaceDescriptor->InterfaceClass != USBCLASS_Audio ) ||
		( msg->InterfaceDescriptor->InterfaceSubClass != 0x02 ) ||
		( msg->InterfaceDescriptor->InterfaceProtocol != 0 ) )
	{
		return ( FALSE );
	}

	driver = (PTR) Self->Data.LibBase;
	ctx = & driver->Context;
	ctx->IUSB2 = msg->IUSB2;
	IUSB2 = msg->IUSB2;

	if ( ! Audio_CreateDevice( ctx ) )
	{
		return ( FALSE );
	}

	unit = Audio_Alloc( ctx, msg );

	if ( ! unit )
	{
		return ( FALSE );
	}

	Audio_Main( unit );
	Audio_Free( unit );

	return ( TRUE );
}

// --
