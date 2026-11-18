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

	if ( ( ! error ) && ( unit->Format.Format == VIDEO_FORMAT_YUY2 ) &&
		( unit->FrameUsed != unit->Format.Width * unit->Format.Height * 2 ) )
	{
		error = IOERR_BADLENGTH;
	}

	unit->ReadActive = NULL;
	ioreq->io_Error = error;
	ioreq->io_Actual = unit->FrameUsed;
	unit->FrameUsed = 0;
	unit->FrameStarted = FALSE;

	ReplyMsg( (PTR) ioreq );
}

// --
