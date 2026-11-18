/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Serial_Manager_AbortIO( struct DeviceManagerInterface *Self, struct IOStdReq *ioreq )
{
struct SerialBase *base;
struct SerialUnit *unit;

	base = (PTR) Self->Data.LibBase;
	unit = (PTR) ioreq->io_Unit;

	ObtainSemaphore( & base->Semaphore );

	if ( ( ! unit ) || ( unit->Detached ) || ( ioreq->io_Message.mn_Node.ln_Type != NT_MESSAGE ) )
	{
		ReleaseSemaphore( & base->Semaphore );
		return;
	}

	ioreq->io_Flags |= IOSERF_ABORT;
	Signal( unit->AbortPort->mp_SigTask,
		( 1UL << unit->AbortPort->mp_SigBit ) );
	ReleaseSemaphore( & base->Semaphore );
}

// --
