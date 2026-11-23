/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Serial_Reply( struct SerialContext *ctx UNUSED, struct IOExtSer *ioreq )
{
	Disable();

	if ( ioreq->IOSer.io_Flags & IOSERF_ABORT )
	{
		ioreq->IOSer.io_Error = IOERR_ABORTED;
	}

	ReplyMsg( (PTR)ioreq );

	Enable();
}

// --
