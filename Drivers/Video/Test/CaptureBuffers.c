
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <exec/errors.h>
#include <exec/libraries.h>
#include <proto/exec.h>
#include <devices/usb2.h>
#include <devices/timer.h>
#include <interfaces/usb2.h>
#include "../inc/VideoDevice.h"
#include "Preview.h"

// --

static void
ReleaseIsoBuffers( struct VideoTestBuffers *buffers )
{
	U32 index;

	if ( buffers->Allocator )
	{
		for ( index = 0; index < VIDEO_ISO_BUFFERS; index++ )
		{
			buffers->Allocator->USB2_FreeIOBuffer( buffers->Iso.Buffer[index] );

			buffers->Iso.Buffer[index] = NULL;
		}

		DropInterface( (PTR) buffers->Allocator );

		buffers->Allocator = NULL;
	}

	if ( buffers->Device.io_Device )
	{
		CloseDevice( & buffers->Device );

		buffers->Device.io_Device = NULL;
	}
}

// --

S32
VideoTest_PrepareIsoBuffers( struct IOStdReq *ioreq, struct VideoTestBuffers *buffers )
{
	U32 index;
	S32 error;

	ioreq->io_Command = VIDEO_CMD_GET_ISO_BUFFER_SIZE;
	ioreq->io_Data = NULL;
	ioreq->io_Length = 0;

	error = DoIO( (PTR) ioreq );

	if ( error == IOERR_NOCMD )
	{
		return ( 0 );
	}

	if ( ( error ) || ( ! ioreq->io_Actual ) )
	{
		return ( error );
	}

	buffers->Iso.Size = ioreq->io_Actual;
	buffers->Device.io_Message.mn_Length = sizeof( buffers->Device );

	if ( OpenDevice( "usb2.device", 0, & buffers->Device, 0 ) )
	{
		return ( 0 );
	}

	if ( ! LIB_IS_AT_LEAST( (struct Library *) buffers->Device.io_Device,
		USB2_IOBUFFER_DEVICE_VERSION, USB2_IOBUFFER_DEVICE_REVISION ) )
	{
		ReleaseIsoBuffers( buffers );

		return ( 0 );
	}

	buffers->Allocator = (PTR) GetInterface( (PTR) buffers->Device.io_Device, "main", 1, NULL );

	if ( ! buffers->Allocator )
	{
		ReleaseIsoBuffers( buffers );

		return ( 0 );
	}

	for ( index = 0; index < VIDEO_ISO_BUFFERS; index++ )
	{
		buffers->Iso.Buffer[index] = buffers->Allocator->USB2_AllocIOBuffer( buffers->Iso.Size );

		if ( ! buffers->Iso.Buffer[index] )
		{
			ReleaseIsoBuffers( buffers );

			return ( 0 );
		}
	}

	ioreq->io_Command = VIDEO_CMD_SET_ISO_BUFFERS;
	ioreq->io_Data = & buffers->Iso;
	ioreq->io_Length = sizeof( buffers->Iso );

	error = DoIO( (PTR) ioreq );

	ioreq->io_Data = NULL;
	ioreq->io_Length = 0;

	if ( error )
	{
		ReleaseIsoBuffers( buffers );
	}

	return ( error );
}

// --

S32
VideoTest_StopCapture( struct IOStdReq *ioreq, struct VideoTestBuffers *buffers )
{
	S32 error;

	ioreq->io_Command = VIDEO_CMD_STOP;
	ioreq->io_Data = NULL;
	ioreq->io_Length = 0;

	error = DoIO( (PTR) ioreq );

	ReleaseIsoBuffers( buffers );

	return ( error );
}

// --
