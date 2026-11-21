
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
	"\0$VER: USB2BluetoothTest 0.7 (21.11.2026)";

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
			Printf( "HCI command $%04lx did not return a valid completion event.\n", (ULONG) error->Opcode );

			break;

		case BT_HCI_NO_SAVED_KEY:
		case BT_HCI_NO_NEW_KEY:
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
	LONG options[5];
	LONG discovery;
	LONG unit;
	LONG result;

	options[0] = 0;
	options[1] = 0;
	options[2] = 0;
	options[3] = 0;
	options[4] = 0;

	args = ReadArgs( "UNIT/K/N,DISCOVER/S,PAIR/K,BOND/K,RECONNECT/K", options, NULL );

	if ( ! args )
	{
		PrintFault( IoErr(), "USB2BluetoothTest" );

		return ( RETURN_FAIL );
	}

	unit = options[0] ? *(const LONG *) options[0] : 0;
	discovery = options[1];

	address = (const char *) options[2];
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

	if ( !!discovery + !!options[2] + !!options[3] + !!options[4] > 1 )
	{
		Printf( "Choose one of DISCOVER, PAIR, BOND, or RECONNECT.\n" );

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
