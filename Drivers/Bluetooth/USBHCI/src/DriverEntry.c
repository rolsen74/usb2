
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <stddef.h>
#include "All.h"
#include <interfaces/bluetooth2share.h>

// --

#define PROBE_HAS( probe, member ) \
	( ( probe )->Message.mn_Length >= offsetof( struct BTControllerProbe, member ) + sizeof( ( probe )->member ) )

static U32 *
ProbeCancelFlag( struct BTControllerProbe *probe )
{
	if ( ( PROBE_HAS( probe, Share ) ) && ( probe->Share ) )
	{
		return ( &probe->Share->Cancel );
	}

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
RunController( struct Bluetooth2IFace *bt2, struct BTUSB *usb, struct MsgPort *requests )
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
			if ( ( PROBE_HAS( probe, Share ) ) && ( probe->Share ) )
			{
				probe->Result = bt2->fileshare_Serve( & transport, probe->Share, probe->Error );
			}
			else if ( ( PROBE_HAS( probe, HID ) ) && ( probe->HID ) )
			{
				if ( probe->HID->Seconds )
				{
					probe->Result = bt2->hid_Monitor( & transport, probe->HID, probe->Error );
				}
				else
				{
					probe->Result = bt2->hidinput_Connect( & transport, (struct BTHIDInput *) probe->HID, probe->Error );
				}
			}
			else if ( ( probe->Message.mn_Length >= offsetof( struct BTControllerProbe, HID ) ) &&
				( probe->PCM ) )
			{
				usb->PrefetchEvents = TRUE;

				probe->Result = bt2->pcm_Play( & transport, probe->PCM, probe->Error );
			}
			else if ( ( probe->Message.mn_Length >= offsetof( struct BTControllerProbe, PCM ) ) &&
				( probe->Audio ) )
			{
				usb->PrefetchEvents = TRUE;

				probe->Result = bt2->a2dp_Play( & transport, probe->Audio, probe->Error );
			}
			else if ( ( probe->Message.mn_Length >= offsetof( struct BTControllerProbe, Audio ) ) &&
				( probe->LE ) )
			{
				probe->Result = bt2->le_Run( & transport.HCI, probe->LE, probe->Error );
			}
			else if ( ( probe->Message.mn_Length >= offsetof( struct BTControllerProbe, LE ) ) &&
				( probe->ObjectPush ) )
			{
				probe->Result = bt2->objectpush_Push( & transport, probe->ObjectPush, probe->Error );
			}
			else if ( ( probe->Message.mn_Length >= offsetof( struct BTControllerProbe, ObjectPush ) ) &&
				( probe->SerialCapture ) )
			{
				probe->Result = bt2->serialcapture_Receive( & transport, probe->SerialCapture, probe->Error );
			}
			else if ( ( probe->Message.mn_Length >= offsetof( struct BTControllerProbe, SerialCapture ) ) &&
				( probe->SerialFile ) )
			{
				probe->Result = bt2->serialfile_Send( & transport, probe->SerialFile, probe->Error );
			}
			else if ( ( probe->Message.mn_Length >= offsetof( struct BTControllerProbe, SerialFile ) ) &&
				( probe->Serial ) )
			{
				probe->Result = bt2->serial_Exchange( & transport, probe->Serial, probe->Error );
			}
			else if ( ( probe->Message.mn_Length >= offsetof( struct BTControllerProbe, Serial ) ) &&
				( probe->RFCOMM ) )
			{
				probe->Result = bt2->rfcomm_Test( & transport, probe->RFCOMM, probe->Error );
			}
			else if ( ( probe->Message.mn_Length >= offsetof( struct BTControllerProbe, RFCOMM ) ) &&
				( probe->Services ) )
			{
				probe->Result = bt2->sdp_Browse( & transport, probe->Services, probe->Error );
			}
			else if ( ( probe->Message.mn_Length >= offsetof( struct BTControllerProbe, Services ) ) &&
				( probe->EchoAddress ) )
			{
				probe->Result = bt2->l2cap_Echo( & transport, probe->EchoAddress, probe->Error );
			}
			else if ( ( probe->Message.mn_Length >= offsetof( struct BTControllerProbe, EchoAddress ) ) &&
				( probe->Visibility ) )
			{
				probe->Result = bt2->visibility_RunVisible( & transport.HCI, probe->Visibility, probe->Error );
			}
			else if ( ( probe->Message.mn_Length >= offsetof( struct BTControllerProbe, Visibility ) ) &&
				( probe->RemoteName ) )
			{
				probe->Result = bt2->names_ReadRemoteName( & transport.HCI, probe->RemoteName, probe->Error );
			}
			else if ( ( probe->Message.mn_Length >= offsetof( struct BTControllerProbe, RemoteName ) ) &&
				( probe->Bond ) )
			{
				probe->Result = bt2->bonding_Bond( & transport.HCI, probe->Bond, probe->Error );
			}
			else if ( ( probe->Message.mn_Length >= offsetof( struct BTControllerProbe, Bond ) ) &&
				( probe->Pair ) )
			{
				probe->Result = bt2->pairing_Pair( & transport.HCI, probe->Pair, probe->Error );
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

		if ( ( probe->Result != BT_HCI_OK ) &&
			( ( PROBE_HAS( probe, Bond ) && probe->Bond ) || ( PROBE_HAS( probe, Pair ) && probe->Pair ) ) )
		{
			DebugPrintF( "Bluetooth USB: pairing failed on if %lu EP $%02lx, HCI $%04lx status $%02lx, result %lu, transport %ld\n",
				(U32) usb->InterfaceNumber, (U32) usb->EventEndpoint, (U32) probe->Error->Opcode,
				(U32) probe->Error->Status, (U32) probe->Result, probe->Error->TransportError );
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
	struct MsgPort *port;
	struct MsgPort *requests;
	struct BTUSB usb;
	struct BTControllerLocation location;
	struct BTHCITransport transport;
	struct BTHCIError error;

	U32 status;
	U32 result;
	S32 unit;
	U8 endpoint;
	U8 recipient;

	if ( ( ! message ) || ( ! message->Interface ) || ( ! message->InterfaceDescriptor ) ||
		( message->InterfaceDescriptor->InterfaceProtocol != 1 ) )
	{
		return ( FALSE );
	}

	// A companion SCO interface has no interrupt-IN HCI event endpoint.
	endpoint = BTUSB_FindEventEndpoint( message, & recipient );

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

	bt2 = (PTR) GetInterface( (PTR) request->io_Device, BLUETOOTH2_INTERFACE, BLUETOOTH2_INTERFACE_VERSION, NULL );

	if ( ! bt2 )
	{
		DebugPrintF( "Bluetooth USB: bluetooth2.device main interface version %lu required\n",
			(U32) BLUETOOTH2_INTERFACE_VERSION );

		goto close_device;
	}

	requests = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! requests )
	{
		goto drop_interface;
	}

	if ( BTUSB_Open( & usb, message, endpoint, recipient ) )
	{
		if ( ! BTUSB_InitializeRealtek( & usb, message->VendorID, message->DeviceID ) )
		{
			goto close_transport;
		}

		transport.Context = & usb;
		transport.StartCommand = BTUSB_StartCommand;
		transport.ReadEventChunk = BTUSB_ReadEventChunk;

		usb.CheckingReadiness = TRUE;

		DebugPrintF( "Bluetooth USB: resetting HCI controller %04lx:%04lx\n",
			(U32) message->VendorID, (U32) message->DeviceID );

		status = bt2->ResetController( & transport, & error );

		usb.CheckingReadiness = FALSE;

		if ( status != BT_HCI_OK )
		{
			DebugPrintF( "Bluetooth USB: HCI readiness check failed for %04lx:%04lx if %lu, event EP $%02lx, "
				"request type $%02lx (result %lu, transport %ld)\n", (U32) message->VendorID,
				(U32) message->DeviceID, (U32) usb.InterfaceNumber, (U32) endpoint,
				(U32) ( REQTYPE_Write | REQTYPE_Class | recipient ), status, error.TransportError );

			goto close_transport;
		}

		location.VendorID = message->VendorID;
		location.ProductID = message->DeviceID;
		location.EventEndpoint = usb.EventEndpoint;

		DebugPrintF( "Bluetooth USB: HCI ready, registering controller %04lx:%04lx\n",
			(U32) location.VendorID, (U32) location.ProductID );

		if ( BTUSB_OpenACL( & usb ) )
		{
			unit = bt2->fileshare_AddController( requests, & location );
		}
		else
		{
			unit = bt2->le_AddController( requests, & location, FALSE );
		}

		if ( unit >= 0 )
		{
			DebugPrintF( "Bluetooth USB: unit %ld attached (%04lx:%04lx)\n",
				unit, (U32) location.VendorID, (U32) location.ProductID );

			RunController( bt2, & usb, requests );

			bt2->RemoveController( unit, requests );

			DebugPrintF( "Bluetooth USB: unit %ld detached (%04lx:%04lx)\n",
				unit, (U32) location.VendorID, (U32) location.ProductID );

			result = TRUE;
		}
		else
		{
			DebugPrintF( "Bluetooth USB: bluetooth2.device refused registration for %04lx:%04lx (result %ld)\n",
				(U32) location.VendorID, (U32) location.ProductID, unit );
		}
	}
	else
	{
		DebugPrintF( "Bluetooth USB: could not open HCI transport for %04lx:%04lx if %lu, event EP $%02lx\n",
			(U32) message->VendorID, (U32) message->DeviceID, (U32) message->Interface->IFC_Num, (U32) endpoint );
	}

close_transport:
	BTUSB_Close( & usb );

	FreeSysObject( ASOT_PORT, requests );

drop_interface:
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
