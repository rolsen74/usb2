/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Serial_Manager_BeginIO( struct DeviceManagerInterface *Self, struct IOStdReq *ioreq )
{
struct SerialBase *base;
struct SerialUnit *unit;

	base = (PTR) Self->Data.LibBase;
	unit = (PTR) ioreq->io_Unit;
	ioreq->io_Flags &= ~IOF_QUICK;
	ioreq->io_Flags &= ~( IOSERF_ABORT | IOSERF_QUEUED | IOSERF_ACTIVE );
	ioreq->io_Error = 0;
	ioreq->io_Actual = 0;
	ioreq->io_Message.mn_Node.ln_Type = NT_MESSAGE;

	ObtainSemaphore( & base->Semaphore );

	if ( ( ! unit ) || ( unit->Detached ) )
	{
		ioreq->io_Error = IOERR_UNITBUSY;
		ReleaseSemaphore( & base->Semaphore );
		ReplyMsg( (PTR) ioreq );

		return;
	}

	PutMsg( unit->BeginPort, (PTR) ioreq );
	ReleaseSemaphore( & base->Semaphore );
}

// --
