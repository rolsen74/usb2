
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
ReportPlayback( struct BTAudioPlayback *audio, enum BTHCIResult result, struct BTHCIError *error )
{
	switch( result )
	{
		case BT_HCI_OK:
			Printf( "Playback finished. Sent %lu PCM sample frames at %lu Hz.\n",
				(ULONG) audio->FramesSent, (ULONG) audio->Rate );

			return ( RETURN_OK );

		case BT_RFCOMM_CANCELLED:
			Printf( "Playback stopped.\n" );

			return ( RETURN_WARN );

		case BT_A2DP_INVALID_WAVE:
			if ( audio->FileError )
			{
				PrintFault( audio->FileError, "PLAY" );
			}
			else
			{
				Printf( "PLAY needs a 16-bit PCM WAV at 44100 or 48000 Hz, mono or stereo.\n" );
			}

			break;

		case BT_A2DP_NO_SINK:
			Printf( "The peer has no available A2DP sink with a compatible SBC configuration.\n" );

			break;

		case BT_A2DP_REJECTED:
			Printf( "The speaker rejected AVDTP signal $%02lx with status $%02lx.\n",
				(ULONG) audio->Signal, (ULONG) audio->Status );

			break;

		case BT_HCI_UNSUPPORTED:
			Printf( "Playback needs cls_Bluetooth.Driver 1.14, a bulk ACL transport, and a compatible speaker.\n" );

			break;

		case BT_HCI_NO_CONTROLLER:
			Printf( "No Bluetooth controller is available on this unit.\n" );

			break;

		case BT_HCI_NO_MEMORY:
		case BT_HCI_TIMER_ERROR:
			Printf( "Could not start the resources for timed Bluetooth playback.\n" );

			break;

		case BT_HCI_BOND_IO_ERROR:
			Printf( "Could not read the saved Bluetooth link key.\n" );

			break;

		case BT_RFCOMM_FILE_ERROR:
			if ( audio->FileError )
			{
				PrintFault( audio->FileError, "PLAY" );
			}
			else
			{
				Printf( "Could not read the complete WAV data.\n" );
			}

			break;

		default:
			ReportHCIError( result, error );

			break;
	}

	return ( RETURN_FAIL );
}

// --

LONG
PlayBluetoothAudio( struct IORequest *request, ULONG unit, const char *address, const char *path )
{
	struct Bluetooth2IFace *interface;
	struct BTAudioPlayback audio;
	struct BTHCIError error;
	enum BTHCIResult result;

	char *resolved;
	BPTR lock;
	LONG status;

	if ( ! ParseAddress( address, audio.Address ) )
	{
		Printf( "SPEAKER requires a Bluetooth address such as 01:23:45:67:89:AB.\n" );

		return ( RETURN_FAIL );
	}

	interface = (APTR) GetInterface( (APTR) request->io_Device, BLUETOOTH2_INTERFACE,
		BLUETOOTH2_INTERFACE_VERSION, NULL );

	if ( ! interface )
	{
		Printf( "This operation requires bluetooth2.device main interface version %lu.\n",
			(ULONG) BLUETOOTH2_INTERFACE_VERSION );

		return ( RETURN_FAIL );
	}

	status = RETURN_FAIL;

	lock = Lock( path, ACCESS_READ );

	if ( ! lock )
	{
		PrintFault( IoErr(), "PLAY" );

		goto drop_interface;
	}

	resolved = AllocVecTags( 4096, TAG_END );

	if ( ! resolved )
	{
		goto release_lock;
	}

	if ( ResolveRFCOMMSource( lock, resolved, 4096 ) )
	{
		audio.Path = resolved;

		Printf( "Playing %s to %s on unit %lu. Ctrl-C stops playback.\n", resolved, address, unit );

		result = interface->a2dp_PlayController( unit, & audio, & error );

		status = ReportPlayback( & audio, result, & error );
	}

	FreeVec( resolved );

release_lock:
	UnLock( lock );

drop_interface:
	DropInterface( (APTR) interface );

	return ( status );
}

// --
