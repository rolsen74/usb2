/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void
Video_Read_Reply( struct VideoUnit *unit, S8 error )
{
struct IOStdReq *ioreq;

	ioreq = unit->ReadActive;

	if ( ! ioreq )
	{
		return;
	}

	if ( unit->TimerPending )
	{
		if ( ( ! CheckIO( & unit->Timer->Request ) ) )
		{
			AbortIO( & unit->Timer->Request );
		}

		if ( ( ! WaitIO( & unit->Timer->Request ) ) && ( ! error ) )
		{
			error = VIDEO_ERR_FRAME_TIMEOUT;
		}

		unit->TimerPending = FALSE;
	}

	ObtainSemaphore( & unit->Base->Semaphore );

	if ( ( unit->Detached ) || ( error == USB2Err_Device_Detached ) )
	{
		error = USB2Err_Device_Detached;
	}
	else if ( ( unit->ReadReset ) || ( ioreq->io_Flags & VIDEO_IO_ABORT ) )
	{
		error = IOERR_ABORTED;
	}

	if ( ( ! error ) && ( unit->Format.Format == VIDEO_FORMAT_YUY2 ) &&
		( unit->FrameUsed != unit->Format.Width * unit->Format.Height * 2 ) )
	{
		error = IOERR_BADLENGTH;
	}

	unit->ReadActive = NULL;
	ioreq->io_Error = error;
	ioreq->io_Actual = error ? 0 : unit->FrameUsed;

	unit->FrameUsed = 0;
	unit->FrameStarted = FALSE;

	if ( error )
	{
		unit->FrameDiscard = TRUE;
	}
	else
	{
		unit->CaptureStats.DeliveredFrames++;
	}

	ReplyMsg( (PTR) ioreq );

	ReleaseSemaphore( & unit->Base->Semaphore );
}

// --
