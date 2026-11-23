
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

#define PROBE_HAS( probe, member ) \
	( ( probe )->Message.mn_Length >= offsetof( struct BTControllerProbe, member ) + sizeof( ( probe )->member ) )

static U32 *
ProbeCancelFlag( struct BTControllerProbe *probe )
{
	if ( ( PROBE_HAS( probe, HID ) ) && ( probe->HID ) )
	{
		return ( & probe->HID->Cancel );
	}
	if ( ( PROBE_HAS( probe, PCM ) ) && ( probe->PCM ) )
	{
		return ( & probe->PCM->Audio.Cancel );
	}
	if ( ( PROBE_HAS( probe, Audio ) ) && ( probe->Audio ) )
	{
		return ( & probe->Audio->Cancel );
	}
	if ( ( PROBE_HAS( probe, LE ) ) && ( probe->LE ) )
	{
		return ( & probe->LE->Cancel );
	}
	if ( ( PROBE_HAS( probe, ObjectPush ) ) && ( probe->ObjectPush ) )
	{
		return ( & probe->ObjectPush->Cancel );
	}
	if ( ( PROBE_HAS( probe, SerialCapture ) ) && ( probe->SerialCapture ) )
	{
		return ( & probe->SerialCapture->Cancel );
	}
	if ( ( PROBE_HAS( probe, SerialFile ) ) && ( probe->SerialFile ) )
	{
		return ( & probe->SerialFile->Cancel );
	}

	return ( NULL );
}

// --

static void
RunController( struct Bluetooth2IFace *bt2, struct Bluetooth2PairingIFace *pairing,
	struct Bluetooth2BondingIFace *bonding, struct Bluetooth2NamesIFace *names,
	struct Bluetooth2VisibilityIFace *visibility, struct Bluetooth2L2CAPIFace *l2cap,
	struct Bluetooth2SDPIFace *sdp, struct Bluetooth2RFCOMMIFace *rfcomm,
	struct Bluetooth2SerialIFace *serial, struct Bluetooth2SerialFileIFace *serialFile,
	struct Bluetooth2SerialCaptureIFace *serialCapture, struct Bluetooth2ObjectPushIFace *objectPush,
	struct Bluetooth2LEIFace *le, struct Bluetooth2A2DPIFace *a2dp,
	struct Bluetooth2PCMIFace *pcm, struct Bluetooth2HIDIFace *hid,
	struct Bluetooth2HIDInputIFace *hidInput, struct BTUSB *usb, struct MsgPort *requests )
{
	struct BTControllerProbe *probe;
	struct BTACLTransport transport;
	struct BTHCIError cleanupError;

	U32 signals;
	U32 mask;
	S32 cancelled;

	usb->RequestSignal = 1UL << requests->mp_SigBit;
	transport.HCI.Context = usb;
	transport.HCI.StartCommand = BTUSB_StartCommand;
	transport.HCI.ReadEventChunk = BTUSB_ReadEventChunk;
	transport.WriteACL = BTUSB_WriteACL;
	transport.ReadACLChunk = BTUSB_ReadACLChunk;
	mask = usb->Registration->Stack_MsgPortBit | usb->RequestSignal | SIGBREAKF_CTRL_C;

	for( ;; )
	{
		if ( BTUSB_StopError( usb ) )
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

		if ( ! PROBE_HAS( probe, Result ) )
		{
			ReplyMsg( & probe->Message );

			continue;
		}

		if ( ( ! PROBE_HAS( probe, Inquiry ) ) || ( ! probe->Error ) )
		{
			probe->Result = BT_HCI_INVALID_ARGUMENT;

			ReplyMsg( & probe->Message );

			continue;
		}

		usb->Cancel = ProbeCancelFlag( probe );

		probe->Result = bt2->ResetController( & transport.HCI, probe->Error );

		if ( probe->Result == BT_HCI_OK )
		{
			if ( ( hid ) && ( probe->Message.mn_Length >= sizeof( *probe ) ) && ( probe->HID ) )
			{
				if ( probe->HID->Seconds )
				{
					probe->Result = hid->Monitor( & transport, probe->HID, probe->Error );
				}
				else if ( hidInput )
				{
					probe->Result = hidInput->Connect( & transport, (struct BTHIDInput *) probe->HID, probe->Error );
				}
				else
				{
					probe->Result = BT_HCI_UNSUPPORTED;
				}
			}
			else if ( ( pcm ) && ( probe->Message.mn_Length >= offsetof( struct BTControllerProbe, HID ) ) &&
				( probe->PCM ) )
			{
				usb->PrefetchEvents = TRUE;

				probe->Result = pcm->Play( & transport, probe->PCM, probe->Error );
			}
			else if ( ( a2dp ) && ( probe->Message.mn_Length >= offsetof( struct BTControllerProbe, PCM ) ) &&
				( probe->Audio ) )
			{
				usb->PrefetchEvents = TRUE;

				probe->Result = a2dp->Play( & transport, probe->Audio, probe->Error );
			}
			else if ( ( le ) && ( probe->Message.mn_Length >= offsetof( struct BTControllerProbe, Audio ) ) &&
				( probe->LE ) )
			{
				probe->Result = le->Run( & transport.HCI, probe->LE, probe->Error );
			}
			else if ( ( objectPush ) && ( probe->Message.mn_Length >= offsetof( struct BTControllerProbe, LE ) ) &&
				( probe->ObjectPush ) )
			{
				probe->Result = objectPush->Push( & transport, probe->ObjectPush, probe->Error );
			}
			else if ( ( serialCapture ) && ( probe->Message.mn_Length >= offsetof( struct BTControllerProbe, ObjectPush ) ) &&
				( probe->SerialCapture ) )
			{
				probe->Result = serialCapture->Receive( & transport, probe->SerialCapture, probe->Error );
			}
			else if ( ( serialFile ) && ( probe->Message.mn_Length >= offsetof( struct BTControllerProbe, SerialCapture ) ) &&
				( probe->SerialFile ) )
			{
				probe->Result = serialFile->Send( & transport, probe->SerialFile, probe->Error );
			}
			else if ( ( serial ) && ( probe->Message.mn_Length >= offsetof( struct BTControllerProbe, SerialFile ) ) &&
				( probe->Serial ) )
			{
				probe->Result = serial->Exchange( & transport, probe->Serial, probe->Error );
			}
			else if ( ( rfcomm ) && ( probe->Message.mn_Length >= offsetof( struct BTControllerProbe, Serial ) ) &&
				( probe->RFCOMM ) )
			{
				probe->Result = rfcomm->Test( & transport, probe->RFCOMM, probe->Error );
			}
			else if ( ( sdp ) && ( probe->Message.mn_Length >= offsetof( struct BTControllerProbe, RFCOMM ) ) &&
				( probe->Services ) )
			{
				probe->Result = sdp->Browse( & transport, probe->Services, probe->Error );
			}
			else if ( ( l2cap ) && ( probe->Message.mn_Length >= offsetof( struct BTControllerProbe, Services ) ) &&
				( probe->EchoAddress ) )
			{
				probe->Result = l2cap->Echo( & transport, probe->EchoAddress, probe->Error );
			}
			else if ( ( visibility ) && ( probe->Message.mn_Length >= offsetof( struct BTControllerProbe, EchoAddress ) ) &&
				( probe->Visibility ) )
			{
				probe->Result = visibility->RunVisible( & transport.HCI, probe->Visibility, probe->Error );
			}
			else if ( ( names ) && ( probe->Message.mn_Length >= offsetof( struct BTControllerProbe, Visibility ) ) &&
				( probe->RemoteName ) )
			{
				probe->Result = names->ReadRemoteName( & transport.HCI, probe->RemoteName, probe->Error );
			}
			else if ( ( bonding ) && ( probe->Message.mn_Length >= offsetof( struct BTControllerProbe, RemoteName ) ) &&
				( probe->Bond ) )
			{
				probe->Result = bonding->Bond( & transport.HCI, probe->Bond, probe->Error );
			}
			else if ( ( pairing ) && ( probe->Message.mn_Length >= offsetof( struct BTControllerProbe, Bond ) ) &&
				( probe->Pair ) )
			{
				probe->Result = pairing->Pair( & transport.HCI, probe->Pair, probe->Error );
			}
			else if ( ( probe->Message.mn_Length >= offsetof( struct BTControllerProbe, Pair ) ) && ( probe->Inquiry ) )
			{
				probe->Result = bt2->Inquiry( & transport.HCI, probe->Inquiry, probe->Error );
			}
			else if ( probe->Info )
			{
				probe->Result = bt2->ReadControllerInfo( & transport.HCI, probe->Info, probe->Error );
			}
			else
			{
				probe->Result = BT_HCI_UNSUPPORTED;
			}
		}

		cancelled = ( usb->Cancel ) && ( __atomic_load_n( usb->Cancel, __ATOMIC_RELAXED ) );

		if ( cancelled )
		{
			probe->Result = ( PROBE_HAS( probe, LE ) && probe->LE ) ? BT_LE_CANCELLED : BT_RFCOMM_CANCELLED;
		}

		usb->Cancel = NULL;
		usb->PrefetchEvents = FALSE;

		if ( ( cancelled ) && ( ! BTUSB_StopError( usb ) ) )
		{
			// A transport-level cancel can interrupt the protocol's disconnect.
			bt2->ResetController( & transport.HCI, & cleanupError );
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
	struct Bluetooth2NamesIFace *names;
	struct Bluetooth2VisibilityIFace *visibility;
	struct Bluetooth2L2CAPIFace *l2cap;
	struct Bluetooth2SDPIFace *sdp;
	struct Bluetooth2RFCOMMIFace *rfcomm;
	struct Bluetooth2SerialIFace *serial;
	struct Bluetooth2SerialFileIFace *serialFile;
	struct Bluetooth2SerialCaptureIFace *serialCapture;
	struct Bluetooth2ObjectPushIFace *objectPush;
	struct Bluetooth2LEIFace *le;
	struct Bluetooth2A2DPIFace *a2dp;
	struct Bluetooth2PCMIFace *pcm;
	struct Bluetooth2HIDIFace *hid;
	struct Bluetooth2HIDInputIFace *hidInput;
	struct MsgPort *port;
	struct MsgPort *requests;
	struct BTUSB usb;
	struct BTControllerLocation location;
	struct BTHCITransport transport;
	struct BTHCIError error;

	U32 status;
	U32 result;
	S32 unit;
	S32 discovery;
	S32 acl;

	if ( ( ! message ) || ( ! message->Interface ) || ( ! message->InterfaceDescriptor ) ||
		( message->InterfaceDescriptor->InterfaceProtocol != 1 ) )
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

	names = (PTR) GetInterface( (PTR) request->io_Device, BLUETOOTH2_NAMES_INTERFACE, BLUETOOTH2_NAMES_VERSION, NULL );

	visibility = (PTR) GetInterface( (PTR) request->io_Device, BLUETOOTH2_VISIBILITY_INTERFACE,
		BLUETOOTH2_VISIBILITY_VERSION, NULL );

	l2cap = (PTR) GetInterface( (PTR) request->io_Device, BLUETOOTH2_L2CAP_INTERFACE, BLUETOOTH2_L2CAP_VERSION, NULL );

	sdp = (PTR) GetInterface( (PTR) request->io_Device, BLUETOOTH2_SDP_INTERFACE, BLUETOOTH2_SDP_VERSION, NULL );

	rfcomm = (PTR) GetInterface( (PTR) request->io_Device, BLUETOOTH2_RFCOMM_INTERFACE, BLUETOOTH2_RFCOMM_VERSION, NULL );

	serial = (PTR) GetInterface( (PTR) request->io_Device, BLUETOOTH2_SERIAL_INTERFACE, BLUETOOTH2_SERIAL_VERSION, NULL );

	serialFile = (PTR) GetInterface( (PTR) request->io_Device, BLUETOOTH2_SERIALFILE_INTERFACE,
		BLUETOOTH2_SERIALFILE_VERSION, NULL );

	serialCapture = (PTR) GetInterface( (PTR) request->io_Device, BLUETOOTH2_CAPTURE_INTERFACE,
		BLUETOOTH2_CAPTURE_VERSION, NULL );

	objectPush = (PTR) GetInterface( (PTR) request->io_Device, BLUETOOTH2_OBEX_INTERFACE,
		BLUETOOTH2_OBEX_VERSION, NULL );

	le = (PTR) GetInterface( (PTR) request->io_Device, BLUETOOTH2_LE_INTERFACE, BLUETOOTH2_LE_VERSION, NULL );

	a2dp = (PTR) GetInterface( (PTR) request->io_Device, BLUETOOTH2_A2DP_INTERFACE, BLUETOOTH2_A2DP_VERSION, NULL );

	pcm = (PTR) GetInterface( (PTR) request->io_Device, BLUETOOTH2_PCM_INTERFACE, BLUETOOTH2_PCM_VERSION, NULL );

	hid = (PTR) GetInterface( (PTR) request->io_Device, BLUETOOTH2_HID_INTERFACE, BLUETOOTH2_HID_VERSION, NULL );

	hidInput = (PTR) GetInterface( (PTR) request->io_Device, BLUETOOTH2_HID_INPUT_INTERFACE,
		BLUETOOTH2_HID_INPUT_VERSION, NULL );

	requests = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! requests )
	{
		goto drop_interface;
	}

	if ( BTUSB_Open( & usb, message ) )
	{
		if ( ! BTUSB_InitializeRealtek( & usb, message->VendorID, message->DeviceID ) )
		{
			goto close_transport;
		}

		transport.Context = & usb;
		transport.StartCommand = BTUSB_StartCommand;
		transport.ReadEventChunk = BTUSB_ReadEventChunk;

		status = bt2->ResetController( & transport, & error );

		if ( status != BT_HCI_OK )
		{
			DebugPrintF( "Bluetooth USB: HCI readiness check failed for %04lx:%04lx (result %lu, transport %ld)\n",
				(U32) message->VendorID, (U32) message->DeviceID, status, error.TransportError );

			goto close_transport;
		}

		location.VendorID = message->VendorID;
		location.ProductID = message->DeviceID;
		location.EventEndpoint = usb.EventEndpoint;

		if ( le )
		{
			acl = BTUSB_OpenACL( & usb );

			if ( ( hid ) && ( pcm ) && ( a2dp ) && ( acl ) )
			{
				unit = hidInput ? hidInput->AddController( requests, & location ) : hid->AddController( requests, & location );
			}
			else if ( ( pcm ) && ( a2dp ) && ( acl ) )
			{
				unit = pcm->AddController( requests, & location );
			}
			else if ( ( a2dp ) && ( acl ) )
			{
				unit = a2dp->AddController( requests, & location );
			}
			else
			{
				unit = le->AddController( requests, & location, acl );
			}
		}
		else if ( ( l2cap ) && ( visibility ) && ( names ) && ( bonding ) && ( pairing ) && ( BTUSB_OpenACL( & usb ) ) )
		{
			if ( ( serial ) && ( rfcomm ) && ( sdp ) )
			{
				if ( ( serialCapture ) && ( serialFile ) )
				{
					unit = objectPush ? objectPush->AddController( requests, & location ) : serialCapture->AddController( requests, & location );
				}
				else
				{
					unit = serialFile ? serialFile->AddController( requests, & location ) : serial->AddController( requests, & location );
				}
			}
			else if ( ( rfcomm ) && ( sdp ) )
			{
				unit = rfcomm->AddController( requests, & location );
			}
			else if ( sdp )
			{
				unit = sdp->AddController( requests, & location );
			}
			else
			{
				unit = l2cap->AddController( requests, & location );
			}
		}
		else if ( ( visibility ) && ( names ) && ( bonding ) && ( pairing ) )
		{
			unit = visibility->AddController( requests, & location );
		}
		else if ( ( names ) && ( bonding ) && ( pairing ) )
		{
			unit = names->AddController( requests, & location );
		}
		else if ( ( bonding ) && ( pairing ) )
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

			RunController( bt2, pairing, bonding, names, visibility, l2cap, sdp, rfcomm, serial,
				serialFile, serialCapture, objectPush, le, a2dp, pcm, hid, hidInput, & usb, requests );

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

close_transport:
	BTUSB_Close( & usb );

	FreeSysObject( ASOT_PORT, requests );

drop_interface:
	if ( hidInput )
	{
		DropInterface( (PTR) hidInput );
	}

	if ( hid )
	{
		DropInterface( (PTR) hid );
	}

	if ( pcm )
	{
		DropInterface( (PTR) pcm );
	}

	if ( a2dp )
	{
		DropInterface( (PTR) a2dp );
	}

	if ( le )
	{
		DropInterface( (PTR) le );
	}

	if ( objectPush )
	{
		DropInterface( (PTR) objectPush );
	}

	if ( serialCapture )
	{
		DropInterface( (PTR) serialCapture );
	}

	if ( serialFile )
	{
		DropInterface( (PTR) serialFile );
	}

	if ( serial )
	{
		DropInterface( (PTR) serial );
	}

	if ( rfcomm )
	{
		DropInterface( (PTR) rfcomm );
	}

	if ( sdp )
	{
		DropInterface( (PTR) sdp );
	}

	if ( l2cap )
	{
		DropInterface( (PTR) l2cap );
	}

	if ( visibility )
	{
		DropInterface( (PTR) visibility );
	}

	if ( names )
	{
		DropInterface( (PTR) names );
	}

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
