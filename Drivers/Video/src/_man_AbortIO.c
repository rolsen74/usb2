/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Video_Manager_AbortIO( struct DeviceManagerInterface *Self, struct IOStdReq *ioreq )
{
struct VideoBase *base;
struct VideoSlot *slot;
struct VideoUnit *unit;

	base = (PTR) Self->Data.LibBase;
	slot = (PTR) ioreq->io_Unit;

	ObtainSemaphore( & base->Semaphore );

	unit = ( slot && slot->Open ) ? slot->Unit : NULL;

	if ( ( unit ) && ( ioreq->io_Message.mn_Node.ln_Type == NT_MESSAGE ) )
	{
		ioreq->io_Flags |= VIDEO_IO_ABORT;
		Signal( unit->BeginPort->mp_SigTask, 1UL << unit->BeginPort->mp_SigBit );
	}

	ReleaseSemaphore( & base->Semaphore );
}

// --
