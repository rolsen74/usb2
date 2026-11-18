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

	while(( ioreq = (PTR) GetMsg( unit->BeginPort ) ))
	{
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
			else
			{
				CopyMem( & unit->Format, ioreq->io_Data, sizeof( struct VideoFormat ) );
				ioreq->io_Actual = sizeof( struct VideoFormat );
			}
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
			else
			{
				unit->ReadActive = ioreq;
				unit->FrameUsed = 0;
				unit->FrameStarted = FALSE;
				// The camera may already be partway through a frame.
				unit->FrameDiscard = TRUE;

				Video_Read_Start( unit );
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
