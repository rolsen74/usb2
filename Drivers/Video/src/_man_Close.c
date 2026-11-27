/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE PTR Video_Manager_Close( struct DeviceManagerInterface *Self, struct IORequest *ioreq )
{
struct VideoBase *base;
struct VideoSlot *slot;
struct VideoUnit *unit;

	base = (PTR) Self->Data.LibBase;
	slot = (PTR) ioreq->io_Unit;

	ObtainSemaphore( & base->Semaphore );

	if ( ( slot ) && ( slot->Open ) )
	{
		unit = slot->Unit;

		if ( unit )
		{
			unit->ReadReset = TRUE;

			Signal( unit->BeginPort->mp_SigTask, 1UL << unit->BeginPort->mp_SigBit );
		}

		slot->Open = FALSE;
		base->Library.lib_OpenCnt--;
		ioreq->io_Unit = NULL;
		ioreq->io_Device = NULL;
	}

	ReleaseSemaphore( & base->Semaphore );

	return ( NULL );
}

// --
