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
		( ( msg->InterfaceDescriptor->InterfaceProtocol != 0 ) &&
		  ( msg->InterfaceDescriptor->InterfaceProtocol != AUDIO_PROTOCOL_UAC2 ) ) )
	{
		return ( FALSE );
	}

	driver = (PTR) Self->Data.LibBase;
	ctx = & driver->Context;
	ctx->IUSB2 = msg->IUSB2;
	IUSB2 = msg->IUSB2;

	AUDIO_TRACE( "%04lx:%04lx interface %lu selected\n",
		(U32) msg->VendorID, (U32) msg->DeviceID, (U32) msg->InterfaceDescriptor->InterfaceNumber );

	if ( ! Audio_CreateDevice( ctx ) )
	{
		AUDIO_TRACE( "could not register usb2audio.device\n" );

		return ( FALSE );
	}

	unit = Audio_Alloc( ctx, msg );

	if ( ! unit )
	{
		return ( FALSE );
	}

	AUDIO_TRACE( "interface %lu ready as unit %lu, endpoint $%02lx, %lu Hz, %lu channel(s)\n",
		(U32) unit->InterfaceNr, unit->UnitNr, (U32) unit->EndPoint,
		unit->Format.SampleRate, unit->Format.Channels );

	Audio_Main( unit );
	Audio_Free( unit );

	return ( TRUE );
}

// --
