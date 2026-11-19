/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Video_Begin( struct VideoUnit *unit )
{
struct IOStdReq *ioreq;
struct VideoMode mode;

	while(( ioreq = (PTR) GetMsg( unit->BeginPort ) ))
	{
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
			else if ( ! unit->Stream )
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
			else if ( ( ! ioreq->io_Offset ) || ( ! Video_Parse( unit, ioreq->io_Offset, & mode ) ) )
			{
				ioreq->io_Error = IOERR_BADADDRESS;
			}
			else
			{
				CopyMem( & mode.Format, ioreq->io_Data, sizeof( mode.Format ) );

				ioreq->io_Actual = sizeof( mode.Format );
			}
		}
		else if ( ioreq->io_Command == VIDEO_CMD_SET_MODE )
		{
			ioreq->io_Error = Video_Set_Mode( unit, ioreq->io_Offset );
		}
		else if ( ioreq->io_Command == VIDEO_CMD_STOP )
		{
			Video_Read_Stop( unit );

			Video_Read_Reply( unit, IOERR_ABORTED );
		}
		else if ( ioreq->io_Command == CMD_READ )
		{
			if ( unit->ReadActive )
			{
				ioreq->io_Error = IOERR_UNITBUSY;
			}
			else if ( ! unit->Stream )
			{
				ioreq->io_Error = IOERR_OPENFAIL;
			}
			else if ( ( ! ioreq->io_Data ) || ( ! ioreq->io_Length ) )
			{
				ioreq->io_Error = IOERR_BADLENGTH;
			}
			else
			{
				unit->ReadActive = ioreq;

				Video_Read_Done( unit );
				continue;
			}
		}
		else
		{
			ioreq->io_Error = IOERR_NOCMD;
		}

		ReplyMsg( (PTR) ioreq );
	}
}

// --
