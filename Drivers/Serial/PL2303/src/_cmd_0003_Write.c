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
	if ( ! ioreq->IOSer.io_Data )
	{
		ioreq->IOSer.io_Error = SerErr_InvParam;
		Serial_Reply( ctx, ioreq );

		return;
	}

	if ( ! ioreq->IOSer.io_Length )
	{
		Serial_Reply( ctx, ioreq );

		return;
	}

	if ( unit->Recovery[SERIAL_PIPE_OUT].Failed )
	{
		unit->Recovery[SERIAL_PIPE_OUT].Failed = FALSE;

		Serial_RecoveryError( ctx, unit, SERIAL_PIPE_OUT, USB2Err_Host_Stall );
	}

	AddTail( & unit->WriteList, (PTR) ioreq );

	Serial_Write_Start( ctx, unit );
}

// --
