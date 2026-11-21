
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <proto/dos.h>
#include <proto/exec.h>
#include "BluetoothTest.h"

// --

static const char version[] __attribute__((used)) =
	"\0$VER: USB2BluetoothTest 0.17 (21.11.2026)";

// --

void
ReportHCIError( enum BTHCIResult result, const struct BTHCIError *error )
{
	switch( result )
	{
		case BT_HCI_WRITE_ERROR:
		case BT_HCI_READ_ERROR:
			Printf( "HCI command $%04lx %s failed (%ld).\n", (ULONG) error->Opcode,
				result == BT_HCI_WRITE_ERROR ? "USB write" : "event read", (LONG) error->TransportError );

			break;

		case BT_HCI_COMMAND_ERROR:
			Printf( "HCI command $%04lx returned status $%02lx.\n",
				(ULONG) error->Opcode, (ULONG) error->Status );

			break;

		case BT_HCI_INVALID_REPLY:
			if ( error->Opcode )
			{
				Printf( "HCI command $%04lx did not return a valid completion event.\n", (ULONG) error->Opcode );
			}
			else
			{
				Printf( "No valid Bluetooth data reply was received.\n" );
			}

			break;

		case BT_ACL_WRITE_ERROR:
		case BT_ACL_READ_ERROR:
			Printf( "Bluetooth ACL %s failed (%ld).\n",
				result == BT_ACL_WRITE_ERROR ? "write" : "read", (LONG) error->TransportError );

			break;

		case BT_L2CAP_REJECTED:
			Printf( "Peer rejected the L2CAP request (reason %lu).\n", (ULONG) error->Status );

			break;

		case BT_RFCOMM_TX_LIMIT:
			Printf( "The peer negotiated basic flow control. This test can send up to %lu bytes in one frame.\n",
				(ULONG) error->Status );

			break;

		case BT_RFCOMM_REJECTED:
			Printf( "The peer rejected or closed the RFCOMM channel (code $%02lx).\n", (ULONG) error->Status );

			break;

		case BT_LE_NOT_SUPPORTED:
		case BT_LE_CANCELLED:
		case BT_OBEX_REJECTED:
		case BT_OBEX_NAME_TOO_LONG:
		case BT_OBEX_SIZE_LIMIT:
		case BT_RFCOMM_FILE_ERROR:
		case BT_RFCOMM_NEEDS_CREDITS:
		case BT_RFCOMM_CANCELLED:
		case BT_SDP_ERROR:
		case BT_HCI_NO_SAVED_KEY:
		case BT_HCI_NO_NEW_KEY:
		case BT_HCI_TIMER_ERROR:
		case BT_HCI_BOND_IO_ERROR:
		case BT_HCI_INVALID_ARGUMENT:
		case BT_HCI_UNSUPPORTED:
		case BT_HCI_NO_CONTROLLER:
		case BT_HCI_NO_MEMORY:
		case BT_HCI_OK:
			break;
	}
}

// --

static LONG
ProbeController( struct Bluetooth2IFace *bt2, ULONG unit )
{
	struct BTControllerLocation location;
	struct BTHCIError error;
	struct BTControllerInfo info;
	enum BTHCIResult result;

	result = bt2->ProbeController( unit, & location, & info, & error );

	if ( result == BT_HCI_NO_CONTROLLER )
	{
		Printf( "No Bluetooth controller available for unit %lu.\n", unit );

		return ( RETURN_FAIL );
	}

	if ( result == BT_HCI_NO_MEMORY )
	{
		Printf( "Could not allocate a Bluetooth probe reply port.\n" );

		return ( RETURN_FAIL );
	}

	Printf( "Bluetooth unit %lu USB %04lx:%04lx, event endpoint $%02lx.\n",
		unit, (ULONG) location.VendorID, (ULONG) location.ProductID, (ULONG) location.EventEndpoint );

	if ( result != BT_HCI_OK )
	{
		ReportHCIError( result, & error );

		return ( RETURN_FAIL );
	}

	Printf( "HCI reset completed.\n" );

	Printf( "HCI version $%02lx, revision $%04lx, LMP version $%02lx, manufacturer %lu, LMP subversion $%04lx.\n",
		(ULONG) info.HCIVersion, (ULONG) info.HCIRevision, (ULONG) info.LMPVersion,
		(ULONG) info.Manufacturer, (ULONG) info.LMPSubversion );

	Printf( "Bluetooth address %02lx:%02lx:%02lx:%02lx:%02lx:%02lx.\n",
		(ULONG) info.Address[0], (ULONG) info.Address[1], (ULONG) info.Address[2],
		(ULONG) info.Address[3], (ULONG) info.Address[4], (ULONG) info.Address[5] );

	return ( RETURN_OK );
}

// --

static LONG
DiscoverController( struct Bluetooth2IFace *bt2, ULONG unit )
{
	struct BTInquiryDevice devices[32];
	struct BTControllerLocation location;
	struct BTInquiryResult inquiry;
	struct BTHCIError error;
	enum BTHCIResult result;

	const uint8_t *address;
	ULONG index;

	inquiry.Devices = devices;
	inquiry.Capacity = sizeof( devices ) / sizeof( devices[0] );

	Printf( "Searching for Classic Bluetooth devices on unit %lu (about five seconds).\n", unit );

	result = bt2->DiscoverController( unit, & location, & inquiry, & error );

	switch( result )
	{
		case BT_HCI_NO_CONTROLLER:
			Printf( "No Bluetooth controller available for unit %lu.\n", unit );

			return ( RETURN_FAIL );

		case BT_HCI_UNSUPPORTED:
			Printf( "Discovery requires Bluetooth.Driver 1.1 or later.\n" );

			return ( RETURN_FAIL );

		case BT_HCI_NO_MEMORY:
			Printf( "Could not allocate a Bluetooth discovery reply port.\n" );

			return ( RETURN_FAIL );

		case BT_HCI_OK:
			break;

		default:
			ReportHCIError( result, & error );

			return ( RETURN_FAIL );
	}

	for( index = 0; index < inquiry.Count; index++ )
	{
		address = devices[index].Address;

		Printf( "%02lx:%02lx:%02lx:%02lx:%02lx:%02lx  class $%06lx\n",
			(ULONG) address[0], (ULONG) address[1], (ULONG) address[2],
			(ULONG) address[3], (ULONG) address[4], (ULONG) address[5], (ULONG) devices[index].ClassOfDevice );
	}

	Printf( "%lu discoverable devices found%s.\n", (ULONG) inquiry.Count,
		inquiry.Truncated ? " (output limit reached)" : "" );

	return ( RETURN_OK );
}

// --

int
main( void )
{
	struct RDArgs *args;
	struct MsgPort *port;
	struct IORequest *request;
	struct Bluetooth2IFace *bt2;

	enum BTPairOperation operation;

	const char *address;
	const char *nameAddress;
	const char *advertisedName;
	const char *echoAddress;
	const char *servicesAddress;
	const char *rfcommAddress;
	const char *text;
	const char *file;
	const char *save;
	const char *push;
	LONG options[22];
	LONG leScan;
	LONG leAdvertise;
	LONG bytes;
	LONG reply;
	LONG channel;
	LONG visible;
	LONG seconds;
	LONG discovery;
	LONG unit;
	LONG result;

	options[0] = 0;
	options[1] = 0;
	options[2] = 0;
	options[3] = 0;
	options[4] = 0;
	options[5] = 0;
	options[6] = 0;
	options[7] = 0;
	options[8] = 0;
	options[9] = 0;
	options[10] = 0;
	options[11] = 0;
	options[12] = 0;
	options[13] = 0;
	options[14] = 0;
	options[15] = 0;
	options[16] = 0;
	options[17] = 0;
	options[18] = 0;
	options[19] = 0;
	options[20] = 0;
	options[21] = 0;

	args = ReadArgs( "UNIT/K/N,DISCOVER/S,PAIR/K,BOND/K,RECONNECT/K,NAME/K,VISIBLE/S,AS/K,SECONDS/K/N,ECHO/K,SERVICES/K,RFCOMM/K,CHANNEL/K/N,TEXT/K,REPLY/S,FILE/K,SAVE/K,BYTES/K/N,PUSH/K,LESCAN/S,LEADVERTISE/S,PASSIVE/S", options, NULL );

	if ( ! args )
	{
		PrintFault( IoErr(), "USB2BluetoothTest" );

		return ( RETURN_FAIL );
	}

	unit = options[0] ? *(const LONG *) options[0] : 0;
	discovery = options[1];

	address = (const char *) options[2];
	nameAddress = (const char *) options[5];
	visible = options[6];
	advertisedName = options[7] ? (const char *) options[7] : "Amiga";
	seconds = options[8] ? *(const LONG *) options[8] : 30;
	echoAddress = (const char *) options[9];
	servicesAddress = (const char *) options[10];
	rfcommAddress = (const char *) options[11];
	channel = options[12] ? *(const LONG *) options[12] : 0;
	text = (const char *) options[13];
	reply = options[14];
	file = (const char *) options[15];
	save = (const char *) options[16];
	bytes = options[17] ? *(const LONG *) options[17] : 0;
	push = (const char *) options[18];
	leScan = options[19];
	leAdvertise = options[20];
	operation = BT_TEST_PAIR;

	if ( options[3] )
	{
		address = (const char *) options[3];
		operation = BT_TEST_BOND;
	}
	else if ( options[4] )
	{
		address = (const char *) options[4];
		operation = BT_TEST_RECONNECT;
	}

	if ( unit < 0 )
	{
		Printf( "UNIT must be zero or greater.\n" );

		FreeArgs( args );

		return ( RETURN_FAIL );
	}

	if ( !!discovery + !!options[2] + !!options[3] + !!options[4] + !!nameAddress + !!visible + !!echoAddress + !!servicesAddress + !!rfcommAddress + !!leScan + !!leAdvertise > 1 )
	{
		Printf( "Choose one of DISCOVER, NAME, PAIR, BOND, RECONNECT, VISIBLE, ECHO, SERVICES, RFCOMM, LESCAN, or LEADVERTISE.\n" );

		FreeArgs( args );

		return ( RETURN_FAIL );
	}

	if ( ( ( options[7] ) && ( ! visible ) && ( ! leAdvertise ) ) ||
		( ( options[8] ) && ( ! visible ) && ( ! leAdvertise ) && ( ! leScan ) ) ||
		( ( options[21] ) && ( ! leScan ) ) )
	{
		Printf( "AS is for VISIBLE or LEADVERTISE. SECONDS also works with LESCAN. PASSIVE requires LESCAN.\n" );

		FreeArgs( args );

		return ( RETURN_FAIL );
	}

	if ( ( ! rfcommAddress ) && ( ( options[12] ) || ( text ) || ( reply ) || ( file ) || ( save ) || ( options[17] ) || ( push ) ) )
	{
		Printf( "CHANNEL, TEXT, REPLY, FILE, SAVE, BYTES, and PUSH require RFCOMM.\n" );

		FreeArgs( args );

		return ( RETURN_FAIL );
	}

	if ( !!file + !!save + !!push + !!( text || reply ) > 1 )
	{
		Printf( "Choose one of FILE, SAVE, PUSH, or TEXT/REPLY.\n" );

		FreeArgs( args );

		return ( RETURN_FAIL );
	}

	if ( ( options[17] ) && ( ! save ) )
	{
		Printf( "BYTES requires SAVE.\n" );

		FreeArgs( args );

		return ( RETURN_FAIL );
	}

	result = RETURN_FAIL;

	port = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! port )
	{
		FreeArgs( args );

		return ( result );
	}

	request = AllocSysObjectTags( ASOT_IOREQUEST, ASOIOR_Size, sizeof( *request ), ASOIOR_ReplyPort, port, TAG_END );

	if ( request )
	{
		if ( OpenDevice( BLUETOOTH2_NAME, 0, request, 0 ) )
		{
			Printf( "Could not open bluetooth2.device.\n" );
		}
		else if ( rfcommAddress )
		{
			if ( push )
			{
				result = PushObjectFile( request, unit, rfcommAddress, channel, push );
			}
			else if ( save )
			{
				result = CaptureRFCOMMFile( request, unit, rfcommAddress, channel, save, bytes );
			}
			else if ( file )
			{
				result = SendRFCOMMFile( request, unit, rfcommAddress, channel, file );
			}
			else
			{
				result = TestRFCOMM( request, unit, rfcommAddress, channel, text, reply );
			}

			CloseDevice( request );
		}
		else if ( servicesAddress )
		{
			result = BrowseServices( request, unit, servicesAddress );

			CloseDevice( request );
		}
		else if ( echoAddress )
		{
			result = L2CAPEcho( request, unit, echoAddress );

			CloseDevice( request );
		}
		else if ( ( leScan ) || ( leAdvertise ) )
		{
			result = RunLE( request, unit, leAdvertise ? BT_LE_ADVERTISE :
				( options[21] ? BT_LE_SCAN_PASSIVE : BT_LE_SCAN_ACTIVE ), advertisedName, seconds );

			CloseDevice( request );
		}
		else if ( visible )
		{
			result = MakeVisible( request, unit, advertisedName, seconds );

			CloseDevice( request );
		}
		else if ( nameAddress )
		{
			result = ReadRemoteName( request, unit, nameAddress );

			CloseDevice( request );
		}
		else if ( address )
		{
			result = PairController( request, unit, address, operation );

			CloseDevice( request );
		}
		else
		{
			bt2 = (APTR) GetInterface( (APTR) request->io_Device, "main", BLUETOOTH2_INTERFACE_VERSION, NULL );

			if ( ( ! bt2 ) && ( ! discovery ) )
			{
				bt2 = (APTR) GetInterface( (APTR) request->io_Device, "main", 2, NULL );
			}

			if ( bt2 )
			{
				result = discovery ? DiscoverController( bt2, unit ) : ProbeController( bt2, unit );

				DropInterface( (APTR) bt2 );
			}
			else
			{
				if ( discovery )
				{
					Printf( "Discovery requires bluetooth2.device main interface version %lu.\n",
						(ULONG) BLUETOOTH2_INTERFACE_VERSION );
				}
				else
				{
					Printf( "Could not obtain bluetooth2.device main interface version 2 or %lu.\n",
						(ULONG) BLUETOOTH2_INTERFACE_VERSION );
				}
			}

			CloseDevice( request );
		}

		FreeSysObject( ASOT_IOREQUEST, request );
	}

	FreeSysObject( ASOT_PORT, port );

	FreeArgs( args );

	return ( result );
}

// --
