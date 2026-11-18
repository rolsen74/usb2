/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE S32 Audio_Manager_Open( struct DeviceManagerInterface *Self, struct IORequest *ioreq, U32 unitnr, U32 flags UNUSED )
{
struct AudioBase *base;
struct AudioUnit *unit;

	base = (PTR) Self->Data.LibBase;
	unit = NULL;

	ObtainSemaphore( & base->Semaphore );

	if ( unitnr < AUDIO_MAX_UNITS )
	{
		unit = base->Units[unitnr];
	}

	if ( ( ! unit ) || ( unit->Detached ) || ( unit->OpenCount ) ||
		( ioreq->io_Message.mn_Length < sizeof( struct IOStdReq ) ) )
	{
		ioreq->io_Error = IOERR_OPENFAIL;
		ioreq->io_Device = NULL;
	}
	else
	{
		base->Library.lib_OpenCnt++;
		unit->OpenCount++;
		ioreq->io_Unit = (PTR) unit;
		ioreq->io_Error = 0;
	}

	ReleaseSemaphore( & base->Semaphore );

	return ( ioreq->io_Error );
}

// --
