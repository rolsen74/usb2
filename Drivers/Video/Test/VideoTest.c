/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <proto/exec.h>
#include <proto/dos.h>
#include <devices/usb2.h>
#include "../inc/VideoDevice.h"

// --

int main( void )
{
struct MsgPort *port;
struct IOStdReq *ioreq;
struct VideoFormat format;

BPTR file;
U8 *frame;
STR name;
S32 result;

	result = RETURN_FAIL;
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

	if ( OpenDevice( VIDEO_DEVICE_NAME, 0, (PTR) ioreq, 0 ) )
	{
		Printf( "No USB video unit 0 is available.\n" );
		goto cleanup;
	}

	ioreq->io_Command = VIDEO_CMD_QUERY;
	ioreq->io_Data = & format;
	ioreq->io_Length = sizeof( format );

	if ( DoIO( (PTR) ioreq ) )
	{
		Printf( "Video format query failed: %ld\n", (S32) ioreq->io_Error );
		goto close_device;
	}

	Printf( "USB %04lx:%04lx, %ld x %ld, format %ld, max frame %ld bytes\n",
		format.VendorID, format.DeviceID, format.Width, format.Height,
		format.Format, format.MaxFrameSize );

	frame = AllocVecTags( format.MaxFrameSize, TAG_END );

	if ( ! frame )
	{
		Printf( "Could not allocate frame buffer.\n" );
		goto close_device;
	}

	ioreq->io_Command = CMD_READ;
	ioreq->io_Data = frame;
	ioreq->io_Length = format.MaxFrameSize;

	if ( DoIO( (PTR) ioreq ) )
	{
		Printf( "Frame read failed: %ld, %ld bytes received\n",
			(S32) ioreq->io_Error, ioreq->io_Actual );
		goto free_frame;
	}

	name = ( format.Format == VIDEO_FORMAT_MJPEG ) ? "RAM:usbvideo.jpg" : "RAM:usbvideo.yuy2";
	file = Open( name, MODE_NEWFILE );

	if ( ! file )
	{
		Printf( "Could not create %s\n", name );
		goto free_frame;
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
