/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Serial_Cmd_Write( struct SerialContext *ctx, struct SerialUnit *unit, struct IOExtSer *ioreq )
{
U8 *data;
U32 length;

	if ( ( unit->Stopped ) || ( ! ioreq->IOSer.io_Data ) )
	{
		ioreq->IOSer.io_Error = SerErr_InvParam;
		Serial_Reply( ctx, ioreq );

		return;
	}

	if ( ioreq->IOSer.io_Length == (U32) -1 )
	{
		data = ioreq->IOSer.io_Data;
		length = 0;

		while( data[length] )
		{
			length++;
		}

		ioreq->IOSer.io_Length = length + 1;
	}

	if ( ! ioreq->IOSer.io_Length )
	{
		Serial_Reply( ctx, ioreq );

		return;
	}

	ioreq->IOSer.io_Flags |= IOSERF_QUEUED;
	AddTail( & unit->WriteList, (PTR) ioreq );

	Serial_Write_Start( ctx, unit );
}

// --
