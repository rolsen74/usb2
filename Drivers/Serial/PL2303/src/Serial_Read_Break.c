/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Serial_Read_Break( struct SerialContext *ctx, struct SerialUnit *unit )
{
struct IOExtSer *ioreq;

	while(( ioreq = (PTR) RemHead( & unit->ReadList ) ))
	{
		ioreq->IOSer.io_Error = SerErr_DetectedBreak;
		Serial_Reply( ctx, ioreq );
	}
}

// --
