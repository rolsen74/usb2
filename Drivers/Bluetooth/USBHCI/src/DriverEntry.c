
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <stddef.h>
#include "All.h"

// --

static S32
Detached( struct USB2_Register *reg )
{
	struct USB2_NotifyMessage *message;

	S32 detached;

	detached = FALSE;

	while(( message = (PTR) GetMsg( reg->Stack_MsgPort ) ))
	{
		if ( message->nm_Command == USBNC_Detached )
		{
			detached = TRUE;
		}

		ReplyMsg( & message->nm_Message );
	}

	return ( detached );
}

// --

static void
RunController( struct Bluetooth2IFace *bt2, struct Bluetooth2PairingIFace *pairing,
	struct Bluetooth2BondingIFace *bonding, struct BTUSB *usb, struct MsgPort *requests )
{
	struct BTControllerProbe *probe;
	struct BTHCITransport transport;

	U32 signals;
	U32 mask;

	transport.Context = usb;
	transport.StartCommand = BTUSB_StartCommand;
	transport.ReadEventChunk = BTUSB_ReadEventChunk;
	mask = usb->Registration->Stack_MsgPortBit | ( 1UL << requests->mp_SigBit ) | SIGBREAKF_CTRL_C;

	for( ;; )
	{
		if ( ( SetSignal( 0, SIGBREAKF_CTRL_C ) & SIGBREAKF_CTRL_C ) || ( Detached( usb->Registration ) ) )
		{
			return;
		}

		probe = (PTR) GetMsg( requests );

		if ( ! probe )
		{
			signals = Wait( mask );

			if ( signals & SIGBREAKF_CTRL_C )
			{
				return;
			}

			continue;
		}

		probe->Result = bt2->ResetController( & transport, probe->Error );

		if ( probe->Result == BT_HCI_OK )
		{
			if ( ( bonding ) && ( probe->Message.mn_Length >= sizeof( *probe ) ) && ( probe->Bond ) )
			{
				probe->Result = bonding->Bond( & transport, probe->Bond, probe->Error );
			}
			else if ( ( pairing ) && ( probe->Message.mn_Length >= offsetof( struct BTControllerProbe, Bond ) ) &&
				( probe->Pair ) )
			{
				probe->Result = pairing->Pair( & transport, probe->Pair, probe->Error );
			}
			else if ( ( probe->Message.mn_Length >= offsetof( struct BTControllerProbe, Pair ) ) && ( probe->Inquiry ) )
			{
				probe->Result = bt2->Inquiry( & transport, probe->Inquiry, probe->Error );
			}
			else
			{
				probe->Result = bt2->ReadControllerInfo( & transport, probe->Info, probe->Error );
			}
		}

		ReplyMsg( & probe->Message );
	}
}

// --

U32
BTUSB_Entry( struct USB2DriverIFace *self __attribute__((unused)), struct USB2_DriverMessage *message )
{
	struct IORequest *request;
	struct Bluetooth2IFace *bt2;
	struct Bluetooth2PairingIFace *pairing;
	struct Bluetooth2BondingIFace *bonding;
	struct MsgPort *port;
	struct MsgPort *requests;
	struct BTUSB usb;
	struct BTControllerLocation location;

	U8 endpoint;
	U32 result;
	S32 unit;
	S32 discovery;

	if ( ( ! message ) || ( ! message->Interface ) || ( ! message->InterfaceDescriptor ) ||
		( message->InterfaceDescriptor->InterfaceProtocol != 1 ) )
	{
		return ( FALSE );
	}

	endpoint = BTUSB_FindEventEndpoint( message->IUSB2, message->Interface );

	if ( ! endpoint )
	{
		return ( FALSE );
	}

	port = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! port )
	{
		return ( FALSE );
	}

	result = FALSE;

	request = AllocSysObjectTags( ASOT_IOREQUEST, ASOIOR_Size, sizeof( *request ), ASOIOR_ReplyPort, port, TAG_END );

	if ( ! request )
	{
		goto free_port;
	}

	if ( OpenDevice( BLUETOOTH2_NAME, 0, request, 0 ) )
	{
		DebugPrintF( "Bluetooth USB: could not open bluetooth2.device\n" );

		goto free_request;
	}

	bt2 = (PTR) GetInterface( (PTR) request->io_Device, "main", BLUETOOTH2_INTERFACE_VERSION, NULL );

	discovery = bt2 != NULL;

	if ( ! bt2 )
	{
		bt2 = (PTR) GetInterface( (PTR) request->io_Device, "main", 2, NULL );
	}

	if ( ! bt2 )
	{
		DebugPrintF( "Bluetooth USB: bluetooth2.device main interface version 2 or later required\n" );

		goto close_device;
	}

	pairing = (PTR) GetInterface( (PTR) request->io_Device, BLUETOOTH2_PAIRING_INTERFACE, BLUETOOTH2_PAIRING_VERSION, NULL );

	bonding = (PTR) GetInterface( (PTR) request->io_Device, BLUETOOTH2_BONDING_INTERFACE, BLUETOOTH2_BONDING_VERSION, NULL );

	requests = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! requests )
	{
		goto drop_interface;
	}

	if ( BTUSB_Open( & usb, message, endpoint ) )
	{
		location.VendorID = message->VendorID;
		location.ProductID = message->DeviceID;
		location.EventEndpoint = endpoint;

		if ( ( bonding ) && ( pairing ) )
		{
			unit = bonding->AddController( requests, & location );
		}
		else if ( pairing )
		{
			unit = pairing->AddController( requests, & location );
		}
		else if ( discovery )
		{
			unit = bt2->AddDiscoveryController( requests, & location );
		}
		else
		{
			unit = bt2->AddController( requests, & location );
		}

		if ( unit >= 0 )
		{
			DebugPrintF( "Bluetooth USB: unit %ld attached (%04lx:%04lx)\n",
				unit, (U32) location.VendorID, (U32) location.ProductID );

			RunController( bt2, pairing, bonding, & usb, requests );

			bt2->RemoveController( unit, requests );

			DebugPrintF( "Bluetooth USB: unit %ld detached (%04lx:%04lx)\n",
				unit, (U32) location.VendorID, (U32) location.ProductID );

			result = TRUE;
		}
	}
	else
	{
		DebugPrintF( "Bluetooth USB: could not claim HCI endpoints for %04lx:%04lx\n",
			(U32) message->VendorID, (U32) message->DeviceID );
	}

	BTUSB_Close( & usb );

	FreeSysObject( ASOT_PORT, requests );

drop_interface:
	if ( bonding )
	{
		DropInterface( (PTR) bonding );
	}

	if ( pairing )
	{
		DropInterface( (PTR) pairing );
	}

	DropInterface( (PTR) bt2 );

close_device:
	CloseDevice( request );

free_request:
	FreeSysObject( ASOT_IOREQUEST, request );

free_port:
	FreeSysObject( ASOT_PORT, port );

	return ( result );
}

// --
