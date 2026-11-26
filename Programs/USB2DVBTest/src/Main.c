
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "AF9015.h"

// --

struct USB2IFace *IUSB2;

static const char Version[] __attribute__((used)) = "$VER: USB2DVBTest 1.0 (25.11.2026)";

enum
{
	ARG_FREQUENCY,
	ARG_BANDWIDTH,
	ARG_FILE,
	ARG_BLOCKS,
	ARG_FIRMWARE,
	ARG_COUNT
};

// --

static S32
RunReceiver( U32 frequency, U32 bandwidth, const char *file, U32 blocks, const char *firmware )
{
	struct Receiver receiver = { 0 };

	S32 result;

	result = RETURN_FAIL;

	if ( ( ! AF9015_Open( & receiver ) ) || ( ! AF9015_Firmware( & receiver, firmware ) ) ||
		( ! AF9015_Board( & receiver ) ) )
	{
		goto bailout;
	}

	if ( ! frequency )
	{
		result = RETURN_OK;
		goto bailout;
	}

	if ( ! AF9015_DemodInit( & receiver ) )
	{
		Printf( "AF9015 demodulator initialization failed.\n" );
	}
	else if ( ( AF9015_Tune( & receiver, frequency, bandwidth ) ) &&
		( AF9015_Capture( & receiver, file, blocks ) ) )
	{
		result = RETURN_OK;
	}

	if ( ( receiver.DemodAwake ) && ( ! AF9015_Detached( & receiver ) ) )
	{
		if ( receiver.TunerDetected )
		{
			AF9015_TunerSleep( & receiver );
		}

		AF9015_DemodSleep( & receiver );
	}

bailout:
	AF9015_Close( & receiver );

	return ( result );
}

// --

int
main( void )
{
	struct RDArgs *args;
	struct MsgPort *port;
	struct IORequest *request;

	LONG options[ARG_COUNT] = { 0 };
	const char *firmware;
	const char *file;
	U32 frequency;
	U32 bandwidth;
	U32 blocks;
	S32 result;
	S32 error;

	port = NULL;
	request = NULL;
	frequency = 0;
	bandwidth = 8;
	blocks = 1;
	firmware = "DEVS:Firmware/dvb-usb-af9015.fw";
	result = RETURN_FAIL;

	args = ReadArgs( "FREQUENCY/K/N,BANDWIDTH/K/N,FILE/K,BLOCKS/K/N,FIRMWARE/K", options, NULL );

	if ( ! args )
	{
		PrintFault( IoErr(), "USB2DVBTest" );

		return ( RETURN_FAIL );
	}

	file = (const char *) options[ARG_FILE];

	if ( file )
	{
		blocks = 256;
	}

	if ( options[ARG_FREQUENCY] )
	{
		frequency = *(LONG *) options[ARG_FREQUENCY];

		if ( ( frequency < 174000000 ) || ( frequency > 864000000 ) )
		{
			Printf( "FREQUENCY must be from 174000000 to 864000000 Hz.\n" );

			goto bailout;
		}
	}
	else if ( ( file ) || ( options[ARG_BANDWIDTH] ) || ( options[ARG_BLOCKS] ) )
	{
		Printf( "Set FREQUENCY in Hz to tune or capture. Run without arguments to identify the tuner.\n" );

		goto bailout;
	}

	if ( options[ARG_BANDWIDTH] )
	{
		bandwidth = *(LONG *) options[ARG_BANDWIDTH];

		if ( ( bandwidth < 6 ) || ( bandwidth > 8 ) )
		{
			Printf( "BANDWIDTH must be 6, 7 or 8 MHz.\n" );

			goto bailout;
		}
	}

	if ( options[ARG_BLOCKS] )
	{
		blocks = *(LONG *) options[ARG_BLOCKS];

		if ( ( ! blocks ) || ( blocks > 65536 ) )
		{
			Printf( "BLOCKS must be from 1 to 65536.\n" );

			goto bailout;
		}
	}

	if ( options[ARG_FIRMWARE] )
	{
		firmware = (const char *) options[ARG_FIRMWARE];
	}

	port = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! port )
	{
		goto bailout;
	}

	request = AllocSysObjectTags( ASOT_IOREQUEST, ASOIOR_Size, sizeof( *request ), ASOIOR_ReplyPort, port, TAG_END );

	if ( ! request )
	{
		goto bailout;
	}

	error = OpenDevice( "usb2.device", 0, request, 0 );

	if ( error )
	{
		Printf( "Could not open usb2.device (%ld).\n", error );

		goto bailout;
	}

	IUSB2 = (PTR) GetInterface( (PTR) request->io_Device, "main", 1, NULL );

	if ( IUSB2 )
	{
		result = RunReceiver( frequency, bandwidth * 1000000, file, blocks, firmware );

		DropInterface( (PTR) IUSB2 );
	}
	else
	{
		Printf( "Could not open the usb2.device main interface.\n" );
	}

	CloseDevice( request );

bailout:
	FreeSysObject( ASOT_IOREQUEST, request );

	FreeSysObject( ASOT_PORT, port );

	FreeArgs( args );

	return ( result );
}

// --
