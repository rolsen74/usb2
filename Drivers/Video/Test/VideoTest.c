/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <stdio.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/errors.h>
#include <devices/usb2.h>
#include <devices/usb2defaults.h>
#include <interfaces/usb2media.h>
#include <devices/timer.h>
#include "../inc/VideoDevice.h"
#include "Preview.h"

// --

#define VIDEO_TEST_FRAMES 30

// --

S32
VideoTest_QueryCamera( struct IOStdReq *ioreq, struct VideoFormat *format )
{
	S32 error;

	ioreq->io_Command = VIDEO_CMD_QUERY;
	ioreq->io_Data = format;
	ioreq->io_Length = sizeof( *format );

	error = DoIO( (PTR) ioreq );

	ioreq->io_Data = NULL;
	ioreq->io_Length = 0;

	return ( error );
}

// --

static S32
FindFirstCamera( struct IOStdReq *ioreq )
{
	struct USB2UnitInfo entries[VIDEO_MAX_UNITS];

	S32 unit;

	unit = -1;

	if ( ! OpenDevice( VIDEO_DEVICE_NAME, USB2_MEDIA_QUERY_UNIT, (PTR) ioreq, 0 ) )
	{
		ioreq->io_Command = USB2_CMD_ENUM_UNITS;
		ioreq->io_Offset = USB2_UNIT_LIST_VERSION;
		ioreq->io_Data = entries;
		ioreq->io_Length = sizeof( entries );

		if ( ( ! DoIO( (PTR) ioreq ) ) && ( ioreq->io_Actual >= sizeof( entries[0] ) ) )
		{
			unit = entries[0].Number;
		}

		CloseDevice( (PTR) ioreq );
	}

	return ( unit );
}

// --

static void
FindCameras( struct IOStdReq *ioreq, struct PreviewCameras *cameras, U32 preferred )
{
	struct VideoFormat format;

	U32 unit;
	U32 index;

	for ( unit = 0; unit < VIDEO_MAX_UNITS; unit++ )
	{
		if ( OpenDevice( VIDEO_DEVICE_NAME, unit, (PTR) ioreq, 0 ) )
		{
			continue;
		}

		if ( ! VideoTest_QueryCamera( ioreq, & format ) )
		{
			index = cameras->Count++;
			cameras->Units[index] = unit;
			cameras->Labels[index] = cameras->Names[index];

			snprintf( cameras->Names[index], sizeof( cameras->Names[index] ), "Camera %lu (%04lx:%04lx)",
				(unsigned long) unit, (unsigned long) format.VendorID, (unsigned long) format.DeviceID );

			if ( unit == preferred )
			{
				cameras->Selected = index;
			}
		}

		CloseDevice( (PTR) ioreq );
	}
}

// --

S32
VideoTest_OpenCamera( struct IOStdReq *ioreq, struct VideoFormat *format, U32 unit )
{
	S32 error;

	error = OpenDevice( VIDEO_DEVICE_NAME, unit, (PTR) ioreq, 0 );

	if ( error )
	{
		Printf( "Could not open %s unit %lu (error %ld).\n", VIDEO_DEVICE_NAME, unit, error );

		return ( FALSE );
	}

	error = VideoTest_QueryCamera( ioreq, format );

	if ( error )
	{
		Printf( "Video format query failed for unit %lu (error %ld).\n", unit, error );

		CloseDevice( (PTR) ioreq );

		return ( FALSE );
	}

	return ( TRUE );
}

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

void
VideoTest_ReportCaptureStats( struct IOStdReq *ioreq )
{
	struct VideoCaptureStats stats;

	ioreq->io_Command = VIDEO_CMD_GET_CAPTURE_STATS;
	ioreq->io_Data = & stats;
	ioreq->io_Length = sizeof( stats );

	if ( ( ! DoIO( (PTR) ioreq ) ) && ( ioreq->io_Actual == sizeof( stats ) ) )
	{
		DebugPrintF( "USB2VideoTest: %lu frames, %lu ISO batches, %lu batch gaps, %lu packet errors, %lu frame length errors\n",
			stats.DeliveredFrames, stats.IsoBatches, stats.IsoBatchGaps, stats.IsoPacketErrors, stats.FrameLengthErrors );
	}

	ioreq->io_Data = NULL;
	ioreq->io_Length = 0;
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
struct TimerIFace *clock;
struct VideoFormat format;
struct PreviewCameras cameras;
struct VideoTestBuffers usbBuffers;

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
U32 rate;
U64 captureStart;
U64 captureEnd;
	S32 written;
	S32 closed;
LONG options[6];
LONG mode;
LONG unit;
S32 setmode;
S32 setunit;
S32 opened;
U32 camera;

	(void) argv;
	options[0] = 0;
	options[1] = 0;
	options[2] = 0;
	options[3] = 0;
	options[4] = 0;
	options[5] = 0;
	mode = 0;
	unit = 0;
	setmode = FALSE;
	setunit = FALSE;
	opened = FALSE;
	result = RETURN_FAIL;
	cameras = (struct PreviewCameras) { 0 };
	usbBuffers = (struct VideoTestBuffers) { 0 };

	if ( argc )
	{
		args = ReadArgs( "SAVE/S,MODES/S,MODE/K/N,UNIT/K/N,CAMERAS/S,TRACE/S", options, NULL );

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

		if ( options[3] )
		{
			unit = *(LONG *) options[3];
			setunit = TRUE;
		}

		FreeArgs( args );
	}

	if ( ( ! setunit ) && ( ! options[4] ) )
	{
		struct USB2Prefs prefs;

		USB2_ReadPrefs( IDOS, & prefs );

		if ( prefs.Units[USB2_DEFAULT_VIDEO] != USB2_UNIT_AUTOMATIC )
		{
			unit = prefs.Units[USB2_DEFAULT_VIDEO];
			setunit = TRUE;
		}
	}

	if ( ( mode < 0 ) || ( ( unit < 0 ) && ( unit != USB2_UNIT_FIRST || options[3] ) ) || ( unit >= VIDEO_MAX_UNITS ) ||
		( ( options[1] ) && ( ( options[0] ) || ( setmode ) ) ) ||
		( ( options[4] ) && ( ( options[0] ) || ( options[1] ) || ( setmode ) || ( setunit ) ) ) )
	{
		Printf( "Use USB2VideoTest CAMERAS, USB2VideoTest [UNIT number] MODES,\n"
			"or USB2VideoTest [UNIT number] [MODE number] [SAVE].\n" );

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

	if ( unit == USB2_UNIT_FIRST )
	{
		unit = FindFirstCamera( ioreq );

		if ( ( unit < 0 ) && ( ! options[0] ) && ( ! options[1] ) && ( ! setmode ) )
		{
			unit = 0;
			setunit = FALSE;
		}
		else if ( unit < 0 )
		{
			Printf( "No first USB video unit is available.\n" );

			goto cleanup;
		}
	}

	if ( ( options[4] ) || ( ( ! options[0] ) && ( ! options[1] ) ) )
	{
		FindCameras( ioreq, & cameras, unit );

		if ( options[4] )
		{
			if ( ! cameras.Count )
			{
				Printf( "No available USB video units.\n" );
			}
			else
			{
				for ( camera = 0; camera < cameras.Count; camera++ )
				{
					Printf( "%s\n", cameras.Labels[camera] );
				}

				result = RETURN_OK;
			}

			goto cleanup;
		}

		if ( ( setunit ) && ( ! options[3] ) && ( ! setmode ) &&
			( ( ! cameras.Count ) || ( cameras.Units[cameras.Selected] != (U32) unit ) ) )
		{
			setunit = FALSE;
		}

		if ( ( ! setunit ) && ( cameras.Count ) )
		{
			unit = cameras.Units[cameras.Selected];
		}
		else if ( ( setunit ) && ( cameras.Units[cameras.Selected] != (U32) unit ) )
		{
			cameras.Count = 0;
		}

		if ( ( ! cameras.Count ) && ( ! setunit ) && ( ! setmode ) )
		{
			format = (struct VideoFormat) { 0 };
			goto open_timer;
		}
	}

	error = OpenDevice( VIDEO_DEVICE_NAME, unit, (PTR) ioreq, 0 );

	if ( error )
	{
		if ( ( ! options[0] ) && ( ! options[1] ) && ( ! options[3] ) && ( ! setmode ) )
		{
			cameras = (struct PreviewCameras) { 0 };
			format = (struct VideoFormat) { 0 };

			goto open_timer;
		}

		Printf( "Could not open %s unit %lu (error %ld).\n", VIDEO_DEVICE_NAME, (U32) unit, error );
		goto cleanup;
	}

	opened = TRUE;

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
			Printf( "Could not set camera mode %ld (error %ld). Run USB2VideoTest UNIT %ld MODES to list modes.\n",
				mode, error, unit );
			goto close_device;
		}
	}

	if ( VideoTest_QueryCamera( ioreq, & format ) )
	{
		Printf( "Video format query failed: %ld\n", (S32) ioreq->io_Error );
		goto close_device;
	}

	Printf( "Unit %ld USB %04lx:%04lx, %ld x %ld, format %ld, max frame %ld bytes, interval %ld (100 ns units)\n",
		unit, format.VendorID, format.DeviceID, format.Width, format.Height,
		format.Format, format.MaxFrameSize, format.FrameInterval );

open_timer:
	frame = NULL;
	clock = NULL;
	captureStart = 0;
	captureEnd = 0;
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
		result = VideoTest_Preview( & ioreq, timer, & format, & cameras, options[5] != 0 );

		goto close_timer;
	}

	frame = AllocVecTags( format.MaxFrameSize, TAG_END );

	if ( ! frame )
	{
		Printf( "Could not allocate frame buffer.\n" );
		goto close_timer;
	}

	error = VideoTest_PrepareIsoBuffers( ioreq, & usbBuffers );

	if ( error )
	{
		Printf( "Could not prepare USB capture buffers (error %ld).\n", error );

		goto close_timer;
	}

	clock = (PTR) GetInterface( (PTR) timer->Request.io_Device, "main", 1, NULL );

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
		timer->Time.Seconds = VIDEO_FRAME_TIMEOUT_SECONDS + 11;
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

		if ( clock )
		{
			captureEnd = VideoTest_UptimeUS( clock );

			if ( ! frameindex )
			{
				captureStart = captureEnd;
			}
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

	if ( captureEnd > captureStart )
	{
		rate = ( VIDEO_TEST_FRAMES - 1 ) * 10000000ULL / ( captureEnd - captureStart );

		Printf( "Capture without preview: %lu.%lu fps.\n", rate / 10, rate % 10 );
	}

	name = ( format.Format == VIDEO_FORMAT_MJPEG ) ? "RAM:usbvideo.jpg" : "RAM:usbvideo.yuy2";
	file = Open( name, MODE_NEWFILE );

	if ( ! file )
	{
		Printf( "Could not create %s\n", name );
		goto close_timer;
	}

	written = Write( file, frame, ioreq->io_Actual );

	closed = Close( file );

	if ( ( written == (S32) ioreq->io_Actual ) && ( closed ) )
	{
		Printf( "Saved %ld bytes to %s\n", ioreq->io_Actual, name );
		result = RETURN_OK;
	}
	else
	{
		Printf( "Could not save the complete frame to %s.\n", name );
	}

close_timer:
	if ( ( options[0] ) && ( options[5] ) )
	{
		VideoTest_ReportCaptureStats( ioreq );
	}

	if ( clock )
	{
		DropInterface( (PTR) clock );
	}

	if ( usbBuffers.Allocator )
	{
		error = VideoTest_StopCapture( ioreq, & usbBuffers );

		if ( ( error ) && ( error != USB2Err_Device_Detached ) )
		{
			Printf( "Could not stop the video stream (error %ld).\n", error );
		}
	}

	if ( options[5] )
	{
		DebugPrintF( "USB2VideoTest: closing timer.device\n" );
	}

	CloseDevice( & timer->Request );

free_timer_request:
	FreeSysObject( ASOT_IOREQUEST, timer );

free_timer_port:
	FreeSysObject( ASOT_PORT, timerport );

free_frame:
	FreeVec( frame );

close_device:
	if ( opened )
	{
		if ( options[5] )
		{
			DebugPrintF( "USB2VideoTest: closing usb2video.device\n" );
		}

		CloseDevice( (PTR) ioreq );
	}

cleanup:
	FreeSysObject( ASOT_IOREQUEST, ioreq );
	FreeSysObject( ASOT_PORT, port );

	if ( options[5] )
	{
		DebugPrintF( "USB2VideoTest: cleanup complete\n" );
	}

	return ( result );
}

// --
