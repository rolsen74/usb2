/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"
#include <stddef.h>
#include <stdint.h>

// --

static S8
SetIsoBuffers( struct VideoUnit *unit, const struct IOStdReq *ioreq )
{
	const struct VideoIsoBuffers *buffers;
	struct VideoIsoBuffers registered;

	uintptr_t first;
	uintptr_t second;
	U32 required;
	U32 count;
	U32 index;
	U32 previous;

	if ( ( unit->Stream ) || ( unit->ReadActive ) )
	{
		return ( IOERR_UNITBUSY );
	}

	if ( ( ! unit->StreamConfigured ) || ( ! unit->StreamAlt ) )
	{
		return ( IOERR_NOCMD );
	}

	required = unit->StreamPacketSize * unit->IsoPacketCount;

	if ( ( ! ioreq->io_Data ) || ( ioreq->io_Length < offsetof( struct VideoIsoBuffers, Buffer[2] ) ) )
	{
		return ( IOERR_BADLENGTH );
	}

	buffers = ioreq->io_Data;

	if ( buffers->Size < required )
	{
		return ( IOERR_BADLENGTH );
	}

	count = ( ioreq->io_Length < sizeof( *buffers ) ) ? 2 : VIDEO_ISO_BUFFERS;
	registered = (struct VideoIsoBuffers) { 0 };
	registered.Size = buffers->Size;

	for ( index = 0; index < count; index++ )
	{
		first = (uintptr_t) buffers->Buffer[index];

		if ( ( ! first ) || ( first > ~(uintptr_t)0 - required ) )
		{
			return ( IOERR_BADADDRESS );
		}

		for ( previous = 0; previous < index; previous++ )
		{
			second = (uintptr_t) registered.Buffer[previous];

			if ( MAX( first, second ) - MIN( first, second ) < required )
			{
				return ( IOERR_BADADDRESS );
			}
		}

		registered.Buffer[index] = buffers->Buffer[index];
	}

	unit->IsoBuffers = registered;

	return ( 0 );
}

// --

SEC_CODE void Video_Begin( struct VideoUnit *unit )
{
struct IOStdReq *ioreq;
struct VideoMode mode;

U32 budget;
S8 error;

	budget = 16;

	while ( ( budget ) && ( ioreq = (PTR) GetMsg( unit->BeginPort ) ) )
	{
		budget--;

		Video_Abort( unit );

		if ( ioreq->io_Flags & VIDEO_IO_ABORT )
		{
			ioreq->io_Error = IOERR_ABORTED;
		}
		else if ( ioreq->io_Command == VIDEO_CMD_QUERY )
		{
			if ( ( ! ioreq->io_Data ) || ( ioreq->io_Length < sizeof( struct VideoFormat ) ) )
			{
				ioreq->io_Error = IOERR_BADLENGTH;
			}
			else if ( ! unit->StreamConfigured )
			{
				ioreq->io_Error = IOERR_OPENFAIL;
			}
			else
			{
				CopyMem( & unit->Format, ioreq->io_Data, sizeof( struct VideoFormat ) );
				ioreq->io_Actual = sizeof( struct VideoFormat );
			}
		}
		else if ( ioreq->io_Command == VIDEO_CMD_GET_MODE )
		{
			if ( ( ! ioreq->io_Data ) || ( ioreq->io_Length < sizeof( struct VideoFormat ) ) )
			{
				ioreq->io_Error = IOERR_BADLENGTH;
			}
			else if ( ( ioreq->io_Offset ) && ( ! Video_Parse( unit, ioreq->io_Offset, & mode ) ) )
			{
				ioreq->io_Error = IOERR_BADADDRESS;
			}
			else
			{
				if ( ! ioreq->io_Offset )
				{
					mode = unit->DefaultMode;
				}

				CopyMem( & mode.Format, ioreq->io_Data, sizeof( mode.Format ) );

				ioreq->io_Actual = sizeof( mode.Format );
			}
		}
		else if ( ioreq->io_Command == VIDEO_CMD_SET_MODE )
		{
			ioreq->io_Error = Video_Set_Mode( unit, ioreq->io_Offset );
		}
		else if ( ioreq->io_Command == VIDEO_CMD_GET_ISO_BUFFER_SIZE )
		{
			if ( ! unit->StreamConfigured )
			{
				ioreq->io_Error = IOERR_OPENFAIL;
			}
			else if ( unit->StreamAlt )
			{
				ioreq->io_Actual = unit->StreamPacketSize * unit->IsoPacketCount;
			}
		}
		else if ( ioreq->io_Command == VIDEO_CMD_SET_ISO_BUFFERS )
		{
			ioreq->io_Error = SetIsoBuffers( unit, ioreq );
		}
		else if ( ioreq->io_Command == VIDEO_CMD_GET_CAPTURE_STATS )
		{
			U32 length;

			length = offsetof( struct VideoCaptureStats, PayloadFrameStarts );

			if ( ( ! ioreq->io_Data ) || ( ioreq->io_Length < length ) )
			{
				ioreq->io_Error = IOERR_BADLENGTH;
			}
			else
			{
				if ( ioreq->io_Length >= sizeof( unit->CaptureStats ) )
				{
					length = sizeof( unit->CaptureStats );
				}

				CopyMem( & unit->CaptureStats, ioreq->io_Data, length );

				ioreq->io_Actual = length;
			}
		}
		else if ( ioreq->io_Command == VIDEO_CMD_STOP )
		{
			error = Video_Stream_Close( unit );

			unit->IsoBuffers.Size = 0;
			ioreq->io_Error = error;

			Video_Read_Reply( unit, error == USB2Err_Device_Detached ? error : IOERR_ABORTED );
		}
		else if ( ioreq->io_Command == CMD_READ )
		{
			if ( unit->ReadActive )
			{
				ioreq->io_Error = IOERR_UNITBUSY;
			}
			else if ( ( ! ioreq->io_Data ) || ( ! ioreq->io_Length ) )
			{
				ioreq->io_Error = IOERR_BADLENGTH;
			}
			else if ( ( error = Video_Stream_Start( unit ) ) )
			{
				ioreq->io_Error = error;
			}
			else
			{
				unit->Timer->Request.io_Command = TR_ADDREQUEST;
				unit->Timer->Time.Seconds = VIDEO_FRAME_TIMEOUT_SECONDS;
				unit->Timer->Time.Microseconds = 0;
				unit->TimerPending = TRUE;

				SendIO( & unit->Timer->Request );

				ObtainSemaphore( & unit->Base->Semaphore );

				unit->ReadActive = ioreq;

				ReleaseSemaphore( & unit->Base->Semaphore );

				Video_Read_Start( unit );

				Signal( unit->StreamPort->mp_SigTask, 1UL << unit->StreamPort->mp_SigBit );
				continue;
			}
		}
		else
		{
			ioreq->io_Error = IOERR_NOCMD;
		}

		ReplyMsg( (PTR) ioreq );
	}

	if ( ! budget )
	{
		Signal( unit->BeginPort->mp_SigTask, 1UL << unit->BeginPort->mp_SigBit );
	}
}

// --
