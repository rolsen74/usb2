/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE PTR Audio_Manager_Close( struct DeviceManagerInterface *Self, struct IORequest *ioreq )
{
struct AudioBase *base;
struct AudioSlot *slot;

	base = (PTR) Self->Data.LibBase;
	slot = (PTR) ioreq->io_Unit;

	ObtainSemaphore( & base->Semaphore );

	base->Library.lib_OpenCnt--;
	ioreq->io_Device = NULL;

	if ( slot )
	{
		slot->Open = FALSE;
		slot->Rate = 0;
		ioreq->io_Unit = NULL;

		if ( slot->Unit )
		{
			slot->Unit->ResetStream = TRUE;
			slot->Unit->ReconnectRate = 0;
		}
	}

	ReleaseSemaphore( & base->Semaphore );

	return ( NULL );
}

// --
