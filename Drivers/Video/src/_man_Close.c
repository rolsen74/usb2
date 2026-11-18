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
struct VideoUnit *unit;

	base = (PTR) Self->Data.LibBase;
	unit = (PTR) ioreq->io_Unit;

	ObtainSemaphore( & base->Semaphore );

	if ( unit )
	{
		unit->OpenCount--;
		base->Library.lib_OpenCnt--;
		ioreq->io_Unit = NULL;
		ioreq->io_Device = NULL;

		if ( ( unit->Released ) && ( ! unit->OpenCount ) )
		{
			FreeVec( unit );
		}
	}

	ReleaseSemaphore( & base->Semaphore );

	return ( NULL );
}

// --
