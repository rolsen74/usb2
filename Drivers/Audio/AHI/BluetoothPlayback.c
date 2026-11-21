
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <dos/var.h>

// --

LONG ParseAddress( const char *text, uint8_t *address );

static struct Playback *ActiveBluetooth;

// --

static LONG
ReadSpeaker( struct Playback *play )
{
	char setting[40];
	const char *text;
	LONG length;
	ULONG unit;
	ULONG digit;

	length = play->DOS->GetVar( "Bluetooth2-AHI", setting, sizeof( setting ), GVF_GLOBAL_ONLY );

	if ( ( length <= 0 ) || ( (ULONG) length >= sizeof( setting ) ) )
	{
		return ( FALSE );
	}

	setting[length] = 0;
	text = setting;
	unit = 0;

	if ( ( *text < '0' ) || ( *text > '9' ) )
	{
		return ( FALSE );
	}

	while( ( *text >= '0' ) && ( *text <= '9' ) )
	{
		digit = *text++ - '0';

		if ( unit > ( 2147483647U - digit ) / 10 )
		{
			return ( FALSE );
		}

		unit = unit * 10 + digit;
	}

	if ( ( *text != ' ' ) && ( *text != '\t' ) )
	{
		return ( FALSE );
	}

	while( ( *text == ' ' ) || ( *text == '\t' ) )
	{
		text++;
	}

	if ( ! ParseAddress( text, play->Stream.Audio.Address ) )
	{
		return ( FALSE );
	}

	play->Unit = unit;

	return ( TRUE );
}

// --

static LONG
OpenBluetoothPlayback( struct Playback *play )
{
	LONG available;

	if ( ! ReadSpeaker( play ) )
	{
		DebugPrintF( "Bluetooth2 AHI: set ENV:Bluetooth2-AHI to unit and speaker address.\n" );

		return ( FALSE );
	}

	Forbid();

	available = ! ActiveBluetooth;

	if ( available )
	{
		ActiveBluetooth = play;
	}

	Permit();

	if ( ! available )
	{
		return ( FALSE );
	}

	if ( OpenDevice( BLUETOOTH2_NAME, 0, (APTR) play->Request, 0 ) )
	{
		play->Request->io_Device = NULL;

		return ( FALSE );
	}

	play->Bluetooth = (APTR) GetInterface( (APTR) play->Request->io_Device,
		BLUETOOTH2_PCM_INTERFACE, BLUETOOTH2_PCM_VERSION, NULL );

	if ( ! play->Bluetooth )
	{
		DebugPrintF( "Bluetooth2 AHI: bluetooth2.device 1.18 or later required.\n" );

		return ( FALSE );
	}

	play->Format.Channels = 2;
	play->Format.SampleRate = DEFAULT_RATE;
	play->Stream.Audio.Rate = play->Format.SampleRate;

	return ( TRUE );
}

// --

static void
CloseBluetoothPlayback( struct Playback *play )
{
	if ( play->Bluetooth )
	{
		DropInterface( (APTR) play->Bluetooth );
	}

	Forbid();

	if ( ActiveBluetooth == play )
	{
		ActiveBluetooth = NULL;
	}

	Permit();
}

// --

static int32_t
ReadMixerPCM( void *context, uint8_t *data, uint32_t length )
{
	struct Playback *play;

	ULONG count;

	play = context;

	if ( play->PCMOffset == play->PCMLength )
	{
		play->PCMOffset = 0;

		play->PCMLength = MixPlayback( play );

		if ( ! play->PCMLength )
		{
			return ( -1 );
		}
	}

	count = play->PCMLength - play->PCMOffset;

	if ( count > length )
	{
		count = length;
	}

	CopyMem( play->PCM + play->PCMOffset, data, count );

	play->PCMOffset += count;

	return ( count );
}

// --

static void
Playback_Run( struct Playback *play )
{
	struct BTHCIError error;
	enum BTHCIResult result;

	error.Opcode = 0;
	error.Status = 0;
	error.TransportError = 0;
	play->PCMOffset = 0;
	play->PCMLength = 0;
	play->Stream.Context = play;
	play->Stream.Read = ReadMixerPCM;

	result = play->Bluetooth->PlayController( play->Unit, & play->Stream, & error );

	if ( ( result != BT_HCI_OK ) && ( result != BT_RFCOMM_CANCELLED ) )
	{
		DebugPrintF( "Bluetooth2 AHI: unit %lu playback failed, result %lu, HCI opcode $%04lx status $%02lx transport %ld, AVDTP $%02lx/$%02lx.\n",
			play->Unit, (ULONG) result, (ULONG) error.Opcode, (ULONG) error.Status, (LONG) error.TransportError,
			(ULONG) play->Stream.Audio.Signal, (ULONG) play->Stream.Audio.Status );
	}
}

// --
