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
	ioreq->IOSer.io_Flags &= ~( IOSERF_QUEUED | IOSERF_ACTIVE );
	ReplyMsg( (PTR) ioreq );
}

// --
