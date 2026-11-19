/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/errors.h>
#include <devices/usb2.h>
#include <devices/timer.h>
#include "../inc/VideoDevice.h"
#include "Preview.h"

// --

#define VIDEO_TEST_FRAMES 30

// --

static S32
ListModes( struct IOStdReq *ioreq )
{
	struct VideoFormat format;

	U32 number;
	U32 fps;
	S32 error;

	for ( number = 1; ; number++ )
	{
		ioreq->io_Command = VIDEO_CMD_GET_MODE;
		ioreq->io_Data = & format;
		ioreq->io_Length = sizeof( format );
		ioreq->io_Offset = number;

		error = DoIO( (PTR) ioreq );

		if ( ( error == IOERR_BADADDRESS ) && ( number > 1 ) )
		{
			return ( RETURN_OK );
		}

		if ( ( error ) || ( ! format.FrameInterval ) )
		{
			Printf( "Could not list camera modes (error %ld).\n", error );

			return ( RETURN_FAIL );
		}

		fps = ( 1000000000UL + format.FrameInterval / 2 ) / (U64) format.FrameInterval;

		Printf( "%3lu  %4lu x %-4lu  %lu.%02lu fps  %s\n", number, format.Width, format.Height,
			fps / 100, fps % 100, ( format.Format == VIDEO_FORMAT_MJPEG ) ? "MJPEG" : "YUY2" );
	}
}

// --

int
main( int argc, char **argv )
{
struct RDArgs *args;
struct MsgPort *port;
struct MsgPort *timerport;
struct IOStdReq *ioreq;
struct TimeRequest *timer;
struct VideoFormat format;

BPTR file;
U8 *frame;
STR name;
S32 result;
S32 error;
S32 stopped;
U32 signals;
U32 mask;
U32 frameindex;
U32 firstsize;
U32 minsize;
U32 maxsize;
LONG options[3];
LONG mode;
S32 setmode;

	(void) argv;
	options[0] = 0;
	options[1] = 0;
	options[2] = 0;
	mode = 0;
	setmode = FALSE;
	result = RETURN_FAIL;

	if ( argc )
	{
		args = ReadArgs( "SAVE/S,MODES/S,MODE/K/N", options, NULL );

		if ( ! args )
		{
			PrintFault( IoErr(), "USB2VideoTest" );

			return ( result );
		}

		if ( options[2] )
		{
			mode = *(LONG *) options[2];
			setmode = TRUE;
		}

		FreeArgs( args );
	}

	if ( ( mode < 0 ) || ( ( options[1] ) && ( ( options[0] ) || ( setmode ) ) ) )
	{
		Printf( "Use USB2VideoTest MODES, or USB2VideoTest [MODE number] [SAVE].\n" );

		return ( result );
	}

	port = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! port )
	{
		return ( result );
	}

	ioreq = (PTR) AllocSysObjectTags( ASOT_IOREQUEST,
		ASOIOR_Size, sizeof( struct IOStdReq ),
		ASOIOR_ReplyPort, port,
		TAG_END );

	if ( ! ioreq )
	{
		FreeSysObject( ASOT_PORT, port );

		return ( result );
	}

	error = OpenDevice( VIDEO_DEVICE_NAME, 0, (PTR) ioreq, 0 );

	if ( error )
	{
		Printf( "Could not open %s unit 0 (error %ld).\n", VIDEO_DEVICE_NAME, error );
		goto cleanup;
	}

	if ( options[1] )
	{
		result = ListModes( ioreq );
		goto close_device;
	}

	if ( setmode )
	{
		ioreq->io_Command = VIDEO_CMD_SET_MODE;
		ioreq->io_Data = NULL;
		ioreq->io_Length = 0;
		ioreq->io_Offset = mode;

		error = DoIO( (PTR) ioreq );

		if ( error )
		{
			Printf( "Could not set camera mode %ld (error %ld). Run USB2VideoTest MODES to list modes.\n",
				mode, error );
			goto close_device;
		}
	}

	ioreq->io_Command = VIDEO_CMD_QUERY;
	ioreq->io_Data = & format;
	ioreq->io_Length = sizeof( format );

	if ( DoIO( (PTR) ioreq ) )
	{
		Printf( "Video format query failed: %ld\n", (S32) ioreq->io_Error );
		goto close_device;
	}

	Printf( "USB %04lx:%04lx, %ld x %ld, format %ld, max frame %ld bytes, interval %ld (100 ns units)\n",
		format.VendorID, format.DeviceID, format.Width, format.Height,
		format.Format, format.MaxFrameSize, format.FrameInterval );

	frame = NULL;
	timerport = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! timerport )
	{
		Printf( "Could not allocate timer port.\n" );
		goto free_frame;
	}

	timer = (PTR) AllocSysObjectTags( ASOT_IOREQUEST,
		ASOIOR_Size, sizeof( *timer ),
		ASOIOR_ReplyPort, timerport,
		TAG_END );

	if ( ! timer )
	{
		Printf( "Could not allocate timer request.\n" );
		goto free_timer_port;
	}

	error = OpenDevice( TIMERNAME, UNIT_MICROHZ, & timer->Request, 0 );

	if ( error )
	{
		Printf( "Could not open timer.device (error %ld).\n", error );
		goto free_timer_request;
	}

	if ( ! options[0] )
	{
		result = VideoTest_Preview( ioreq, timer, & format );
		goto close_timer;
	}

	frame = AllocVecTags( format.MaxFrameSize, TAG_END );

	if ( ! frame )
	{
		Printf( "Could not allocate frame buffer.\n" );
		goto close_timer;
	}

	mask = ( 1UL << port->mp_SigBit ) | ( 1UL << timerport->mp_SigBit ) | SIGBREAKF_CTRL_C;
	firstsize = 0;
	minsize = 0;
	maxsize = 0;

	Printf( "Capturing %lu frames, saving the last (10 second timeout per frame)...\n",
		(U32) VIDEO_TEST_FRAMES );

	for( frameindex = 0; frameindex < VIDEO_TEST_FRAMES; frameindex++ )
	{
		ioreq->io_Command = CMD_READ;
		ioreq->io_Data = frame;
		ioreq->io_Length = format.MaxFrameSize;
		timer->Request.io_Command = TR_ADDREQUEST;
		timer->Request.io_Flags = 0;
		timer->Time.Seconds = 10;
		timer->Time.Microseconds = 0;
		stopped = FALSE;

		SendIO( & timer->Request );
		SendIO( (PTR) ioreq );

		while( ( ! CheckIO( (PTR) ioreq ) ) && ( ! CheckIO( & timer->Request ) ) )
		{
			signals = Wait( mask );

			if ( signals & SIGBREAKF_CTRL_C )
			{
				Printf( "Capture interrupted.\n" );
				stopped = TRUE;
				break;
			}
		}

		if ( ! CheckIO( (PTR) ioreq ) )
		{
			if ( CheckIO( & timer->Request ) )
			{
				Printf( "Frame %lu timed out.\n", frameindex + 1 );
			}

			stopped = TRUE;

			AbortIO( (PTR) ioreq );
			WaitIO( (PTR) ioreq );
		}
		else
		{
			WaitIO( (PTR) ioreq );
		}

		if ( ! CheckIO( & timer->Request ) )
		{
			AbortIO( & timer->Request );
		}

		WaitIO( & timer->Request );

		if ( stopped )
		{
			goto close_timer;
		}

		if ( ioreq->io_Error )
		{
			Printf( "Frame %lu read failed: %ld, %ld bytes received\n",
				frameindex + 1, (S32) ioreq->io_Error, ioreq->io_Actual );
			goto close_timer;
		}

		if ( ! frameindex )
		{
			firstsize = ioreq->io_Actual;
			minsize = ioreq->io_Actual;
			maxsize = ioreq->io_Actual;
		}
		else
		{
			if ( ioreq->io_Actual < minsize )
			{
				minsize = ioreq->io_Actual;
			}

			if ( ioreq->io_Actual > maxsize )
			{
				maxsize = ioreq->io_Actual;
			}
		}
	}

	Printf( "Frame sizes: first %lu, last %lu, range %lu-%lu bytes.\n",
		firstsize, ioreq->io_Actual, minsize, maxsize );

	name = ( format.Format == VIDEO_FORMAT_MJPEG ) ? "RAM:usbvideo.jpg" : "RAM:usbvideo.yuy2";
	file = Open( name, MODE_NEWFILE );

	if ( ! file )
	{
		Printf( "Could not create %s\n", name );
		goto close_timer;
	}

	if ( Write( file, frame, ioreq->io_Actual ) == (S32) ioreq->io_Actual )
	{
		Printf( "Saved %ld bytes to %s\n", ioreq->io_Actual, name );
		result = RETURN_OK;
	}
	else
	{
		Printf( "Could not write complete frame.\n" );
	}

	Close( file );

close_timer:
	CloseDevice( & timer->Request );

free_timer_request:
	FreeSysObject( ASOT_IOREQUEST, timer );

free_timer_port:
	FreeSysObject( ASOT_PORT, timerport );

free_frame:
	FreeVec( frame );

close_device:
	CloseDevice( (PTR) ioreq );

cleanup:
	FreeSysObject( ASOT_IOREQUEST, ioreq );
	FreeSysObject( ASOT_PORT, port );

	return ( result );
}

// --
