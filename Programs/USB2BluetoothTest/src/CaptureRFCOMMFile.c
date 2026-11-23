
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include "BluetoothTest.h"

// --

#define CAPTURE_PATH_SIZE 4096

// --

static LONG
ResolveDestination( const char *path, char *resolved )
{
	const char *leaf;
	const char *parentEnd;
	char *parent;
	ULONG prefix;
	BPTR lock;
	LONG result;

	leaf = FilePart( path );

	parentEnd = PathPart( path );

	if ( ( ! leaf ) || ( ! *leaf ) || ( ! parentEnd ) )
	{
		Printf( "SAVE requires a destination filename.\n" );

		return ( FALSE );
	}

	prefix = parentEnd - path;

	parent = AllocVecTags( prefix + 1, TAG_END );

	if ( ! parent )
	{
		Printf( "Could not allocate the destination path buffer.\n" );

		return ( FALSE );
	}

	memcpy( parent, path, prefix );

	parent[prefix] = 0;

	lock = Lock( parent, ACCESS_READ );

	result = FALSE;

	if ( lock )
	{
		result = NameFromLock( lock, resolved, CAPTURE_PATH_SIZE ) &&
			AddPart( resolved, leaf, CAPTURE_PATH_SIZE );
	}

	if ( ! result )
	{
		PrintFault( IoErr(), "SAVE" );
	}

	if ( lock )
	{
		UnLock( lock );
	}

	FreeVec( parent );

	return ( result );
}

// --

static LONG
ReportCaptureResult( const struct BTRFCOMMCapture *capture, enum BTHCIResult result,
	const struct BTHCIError *error, ULONG unit )
{
	if ( result == BT_HCI_OK )
	{
		printf( "Saved %" PRIu64 " bytes to %s.\n", capture->Received, capture->Path );

		fflush( stdout );

		return ( RETURN_OK );
	}

	if ( ( capture->Received ) || ( result == BT_RFCOMM_CANCELLED ) )
	{
		printf( "Received %" PRIu64 " of %" PRIu64 " bytes. The destination was not created.\n",
			capture->Received, capture->Expected );

		fflush( stdout );
	}

	switch( result )
	{
		case BT_RFCOMM_CANCELLED:
			Printf( "File capture cancelled.\n" );

			return ( RETURN_WARN );

		case BT_RFCOMM_FILE_ERROR:
			if ( capture->FileError )
			{
				PrintFault( capture->FileError, "SAVE" );
			}
			else
			{
				Printf( "Could not write the capture file.\n" );
			}

			break;

		case BT_RFCOMM_NEEDS_CREDITS:
			Printf( "SAVE requires a peer that negotiates RFCOMM credit-based flow control.\n" );

			break;

		case BT_HCI_NO_CONTROLLER:
			Printf( "No Bluetooth controller available for unit %lu.\n", unit );

			break;

		case BT_HCI_NO_MEMORY:
			Printf( "Could not allocate Bluetooth capture resources.\n" );

			break;

		case BT_HCI_UNSUPPORTED:
			Printf( "SAVE requires cls_Bluetooth.Driver 1.11 and a Basic Mode L2CAP channel.\n" );

			break;

		case BT_HCI_BOND_IO_ERROR:
			Printf( "Could not read the saved Bluetooth link key.\n" );

			break;

		default:
			ReportRFCOMMStage( capture->Peer.Stage );

			ReportHCIError( result, error );

			break;
	}

	return ( RETURN_FAIL );
}

// --

LONG
CaptureRFCOMMFile( struct IORequest *request, ULONG unit, const char *address, LONG channel,
	const char *path, LONG bytes )
{
	struct Bluetooth2SerialCaptureIFace *interface;
	struct BTRFCOMMCapture capture;
	struct BTHCIError error;
	enum BTHCIResult result;

	char *resolved;
	LONG status;

	if ( ! ParseAddress( address, capture.Peer.Address ) )
	{
		Printf( "RFCOMM requires a Bluetooth address such as 01:23:45:67:89:AB.\n" );

		return ( RETURN_FAIL );
	}

	if ( ( channel < 1 ) || ( channel > 30 ) )
	{
		Printf( "Enter CHANNEL=1 through CHANNEL=30. SERVICES can show the peer's channel.\n" );

		return ( RETURN_FAIL );
	}

	if ( bytes < 1 )
	{
		Printf( "SAVE requires BYTES with a positive byte count.\n" );

		return ( RETURN_FAIL );
	}

	interface = (APTR) GetInterface( (APTR) request->io_Device, BLUETOOTH2_CAPTURE_INTERFACE,
		BLUETOOTH2_CAPTURE_VERSION, NULL );

	if ( ! interface )
	{
		Printf( "SAVE requires Bluetooth2 1.13 or later.\n" );

		return ( RETURN_FAIL );
	}

	status = RETURN_FAIL;

	resolved = AllocVecTags( CAPTURE_PATH_SIZE, TAG_END );

	if ( resolved )
	{
		if ( ResolveDestination( path, resolved ) )
		{
			capture.Peer.Channel = channel;
			capture.Path = resolved;
			capture.Expected = bytes;

			Printf( "Capturing %ld bytes from %s on RFCOMM channel %ld. Ctrl-C cancels.\n", bytes, address, channel );

			result = interface->ReceiveController( unit, & capture, & error );

			status = ReportCaptureResult( & capture, result, & error, unit );
		}

		FreeVec( resolved );
	}
	else
	{
		Printf( "Could not allocate the destination path buffer.\n" );
	}

	DropInterface( (APTR) interface );

	return ( status );
}

// --
