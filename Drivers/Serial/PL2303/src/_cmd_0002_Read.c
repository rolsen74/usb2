/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Serial_Cmd_Read( struct SerialContext *ctx, struct SerialUnit *unit, struct IOExtSer *ioreq )
{
	if ( ( ! ioreq->IOSer.io_Data ) || ( ! ioreq->IOSer.io_Length ) )
	{
		ioreq->IOSer.io_Error = SerErr_InvParam;
		Serial_Reply( ctx, ioreq );

		return;
	}

	if ( unit->Recovery[SERIAL_PIPE_IN].Failed )
	{
		unit->Recovery[SERIAL_PIPE_IN].Failed = FALSE;

		Serial_RecoveryError( ctx, unit, SERIAL_PIPE_IN,
			unit->Recovery[SERIAL_PIPE_IN].Stalled ? USB2Err_Host_Stall : USB2Err_Host_Timeout );
	}

	AddTail( & unit->ReadList, (PTR) ioreq );

	Serial_Read_Drain( ctx, unit );
}

// --
