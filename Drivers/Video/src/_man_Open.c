/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"
#include <interfaces/usb2units.h>

// --

SEC_CODE S32 Video_Manager_Open( struct DeviceManagerInterface *Self, struct IORequest *ioreq, U32 unitnr, U32 flags UNUSED )
{
struct VideoBase *base;
struct VideoUnit *unit;

U32 index;

	base = (PTR) Self->Data.LibBase;
	unit = NULL;

	ObtainSemaphore( & base->Semaphore );

	if ( unitnr == USB2_FIRST_UNIT )
	{
		for( index = 0; index < VIDEO_MAX_UNITS; index++ )
		{
			if ( ( base->Units[index] ) && ( ! base->Units[index]->Detached ) )
			{
				unit = base->Units[index];

				break;
			}
		}
	}
	else if ( unitnr < VIDEO_MAX_UNITS )
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
