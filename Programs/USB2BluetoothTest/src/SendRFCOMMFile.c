
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <inttypes.h>
#include <stdio.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include "BluetoothTest.h"

// --

LONG
ResolveRFCOMMSource( BPTR lock, char *path, ULONG length )
{
	struct ExamineData *info;

	LONG regular;

	info = ExamineObjectTags( EX_FileLockInput, lock, TAG_END );

	if ( ! info )
	{
		PrintFault( IoErr(), "FILE" );

		return ( FALSE );
	}

	regular = EXD_IS_FILE( info );

	FreeDosObject( DOS_EXAMINEDATA, info );

	if ( ! regular )
	{
		Printf( "FILE requires a regular file.\n" );

		return ( FALSE );
	}

	if ( ! NameFromLock( lock, path, length ) )
	{
		PrintFault( IoErr(), "FILE" );

		return ( FALSE );
	}

	return ( TRUE );
}

// --

static LONG
ReportFileResult( const struct BTRFCOMMFile *file, enum BTHCIResult result, const struct BTHCIError *error, ULONG unit )
{
	if ( ( file->Sent ) || ( result == BT_HCI_OK ) || ( result == BT_RFCOMM_CANCELLED ) )
	{
		printf( "Sent %" PRIu64 " bytes.\n", file->Sent );

		fflush( stdout );
	}

	switch( result )
	{
		case BT_HCI_OK:
			return ( RETURN_OK );

		case BT_RFCOMM_CANCELLED:
			Printf( "File send cancelled.\n" );

			return ( RETURN_WARN );

		case BT_RFCOMM_FILE_ERROR:
			if ( file->FileError )
			{
				PrintFault( file->FileError, "FILE" );
			}
			else
			{
				Printf( "Could not read the source file.\n" );
			}

			break;

		case BT_RFCOMM_NEEDS_CREDITS:
			Printf( "FILE requires a peer that negotiates RFCOMM credit-based flow control.\n" );

			break;

		case BT_HCI_NO_CONTROLLER:
			Printf( "No Bluetooth controller available for unit %lu.\n", unit );

			break;

		case BT_HCI_NO_MEMORY:
			Printf( "Could not allocate a Bluetooth file reply port.\n" );

			break;

		case BT_HCI_UNSUPPORTED:
			Printf( "FILE requires cls_Bluetooth.Driver 1.10 and a Basic Mode L2CAP channel.\n" );

			break;

		case BT_HCI_BOND_IO_ERROR:
			Printf( "Could not read the saved Bluetooth link key.\n" );

			break;

		default:
			ReportRFCOMMStage( file->Peer.Stage );

			ReportHCIError( result, error );

			break;
	}

	return ( RETURN_FAIL );
}

// --

LONG
SendRFCOMMFile( struct IORequest *request, ULONG unit, const char *address, LONG channel, const char *path )
{
	struct Bluetooth2SerialFileIFace *serial;
	struct BTRFCOMMFile file;
	struct BTHCIError error;
	enum BTHCIResult result;

	char *resolved;
	BPTR lock;
	LONG status;

	if ( ! ParseAddress( address, file.Peer.Address ) )
	{
		Printf( "RFCOMM requires a Bluetooth address such as 01:23:45:67:89:AB.\n" );

		return ( RETURN_FAIL );
	}

	if ( ( channel < 1 ) || ( channel > 30 ) )
	{
		Printf( "Enter CHANNEL=1 through CHANNEL=30. SERVICES can show the peer's channel.\n" );

		return ( RETURN_FAIL );
	}

	serial = (APTR) GetInterface( (APTR) request->io_Device, BLUETOOTH2_SERIALFILE_INTERFACE,
		BLUETOOTH2_SERIALFILE_VERSION, NULL );

	if ( ! serial )
	{
		Printf( "FILE requires Bluetooth2 1.12 or later.\n" );

		return ( RETURN_FAIL );
	}

	status = RETURN_FAIL;

	lock = Lock( path, ACCESS_READ );

	if ( ! lock )
	{
		PrintFault( IoErr(), "FILE" );

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
		file.Peer.Channel = channel;
		file.Path = resolved;

		Printf( "Sending %s to %s on RFCOMM channel %ld. Ctrl-C cancels.\n", resolved, address, channel );

		result = serial->SendController( unit, & file, & error );

		status = ReportFileResult( & file, result, & error, unit );
	}

	FreeVec( resolved );

release_lock:
	UnLock( lock );

drop_interface:
	DropInterface( (APTR) serial );

	return ( status );
}

// --
