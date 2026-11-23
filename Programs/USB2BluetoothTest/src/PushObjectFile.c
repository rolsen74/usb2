
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

static LONG
ReportPushResult( const struct BTObjectPush *push, enum BTHCIResult result, const struct BTHCIError *error, ULONG unit )
{
	if ( result == BT_HCI_OK )
	{
		Printf( "The peer accepted the file (%lu bytes).\n", (ULONG) push->Sent );

		return ( RETURN_OK );
	}

	switch( result )
	{
		case BT_RFCOMM_CANCELLED:
			Printf( "Object Push cancelled after %lu acknowledged bytes.\n", (ULONG) push->Sent );

			return ( RETURN_WARN );

		case BT_OBEX_NO_SERVICE:
			Printf( "No Object Push receiver was found. Open the receiving device's Bluetooth file receiver.\n" );

			break;

		case BT_OBEX_REJECTED:
			Printf( "The peer rejected the OBEX request with response $%02lx.\n", (ULONG) push->Response );

			break;

		case BT_OBEX_NAME_TOO_LONG:
			Printf( "The filename does not fit in the peer's OBEX packet. Try a shorter name.\n" );

			break;

		case BT_OBEX_SIZE_LIMIT:
			Printf( "The file exceeds the 32-bit OBEX Length field.\n" );

			break;

		case BT_RFCOMM_FILE_ERROR:
			if ( push->FileError )
			{
				PrintFault( push->FileError, "PUSH" );
			}
			else
			{
				Printf( "Could not read the complete source file.\n" );
			}

			break;

		case BT_RFCOMM_NEEDS_CREDITS:
			Printf( "PUSH requires a peer that negotiates RFCOMM credit-based flow control.\n" );

			break;

		case BT_HCI_NO_CONTROLLER:
			Printf( "No Bluetooth controller available for unit %lu.\n", unit );

			break;

		case BT_HCI_NO_MEMORY:
			Printf( "Could not allocate Bluetooth Object Push resources.\n" );

			break;

		case BT_HCI_UNSUPPORTED:
			Printf( "PUSH requires cls_Bluetooth.Driver 1.12 and a Basic Mode L2CAP channel.\n" );

			break;

		case BT_HCI_BOND_IO_ERROR:
			Printf( "Could not read the saved Bluetooth link key.\n" );

			break;

		default:
			ReportRFCOMMStage( push->Peer.Stage );

			ReportHCIError( result, error );

			break;
	}

	return ( RETURN_FAIL );
}

// --

LONG
PushObjectFile( struct IORequest *request, ULONG unit, const char *address, LONG channel, const char *path )
{
	struct Bluetooth2ObjectPushIFace *interface;
	struct BTObjectPush push;
	struct BTHCIError error;
	enum BTHCIResult result;

	char *resolved;
	BPTR lock;
	LONG status;

	if ( ! ParseAddress( address, push.Peer.Address ) )
	{
		Printf( "RFCOMM requires a Bluetooth address such as 01:23:45:67:89:AB.\n" );

		return ( RETURN_FAIL );
	}

	if ( ( channel < 0 ) || ( channel > 30 ) )
	{
		Printf( "CHANNEL must be 1 to 30, or omit it for automatic service discovery.\n" );

		return ( RETURN_FAIL );
	}

	if ( ( ! channel ) && ( request->io_Device->dd_Library.lib_Version == 1 ) &&
		( request->io_Device->dd_Library.lib_Revision < 16 ) )
	{
		Printf( "Automatic Object Push discovery requires Bluetooth2 1.16. For earlier versions, specify CHANNEL.\n" );

		return ( RETURN_FAIL );
	}

	interface = (APTR) GetInterface( (APTR) request->io_Device, BLUETOOTH2_OBEX_INTERFACE,
		BLUETOOTH2_OBEX_VERSION, NULL );

	if ( ! interface )
	{
		Printf( "PUSH requires Bluetooth2 1.14 or later.\n" );

		return ( RETURN_FAIL );
	}

	status = RETURN_FAIL;

	lock = Lock( path, ACCESS_READ );

	if ( ! lock )
	{
		PrintFault( IoErr(), "PUSH" );

		goto drop_interface;
	}

	resolved = AllocVecTags( 4096, TAG_END );

	if ( ! resolved )
	{
		Printf( "Could not allocate the source path buffer.\n" );

		goto release_lock;
	}

	if ( ResolveRFCOMMSource( lock, resolved, 4096 ) )
	{
		push.Peer.Channel = channel;
		push.Path = resolved;

		if ( channel )
		{
			Printf( "Pushing %s to %s on RFCOMM channel %ld. Ctrl-C cancels.\n", resolved, address, channel );
		}
		else
		{
			Printf( "Finding the Object Push receiver on %s. Ctrl-C cancels.\n", address );
		}

		result = interface->PushController( unit, & push, & error );

		status = ReportPushResult( & push, result, & error, unit );
	}

	FreeVec( resolved );

release_lock:
	UnLock( lock );

drop_interface:
	DropInterface( (APTR) interface );

	return ( status );
}

// --
