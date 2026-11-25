
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

SEC_CODE void
Video_EnumUnits( struct VideoBase *base, struct IOStdReq *request )
{
	struct VideoUnit *unit;
	struct USB2UnitInfo entries[VIDEO_MAX_UNITS];

	U32 count;
	U32 index;

	if ( request->io_Offset != USB2_UNIT_LIST_VERSION )
	{
		request->io_Error = IOERR_NOCMD;

		return;
	}

	count = 0;

	ObtainSemaphore( & base->Semaphore );

	for( index = 0; index < VIDEO_MAX_UNITS; index++ )
	{
		unit = base->Units[index];

		if ( ( unit ) && ( ! unit->Detached ) )
		{
			entries[count].Number = index;
			entries[count].Flags = unit->OpenCount ? USB2_UNIT_BUSY : 0;
			count++;
		}
	}

	ReleaseSemaphore( & base->Semaphore );

	request->io_Actual = count * sizeof( entries[0] );

	if ( ( request->io_Actual > request->io_Length ) || ( ( count ) && ( ! request->io_Data ) ) )
	{
		request->io_Error = IOERR_BADLENGTH;
	}
	else if ( count )
	{
		CopyMem( entries, request->io_Data, request->io_Actual );
	}
}

// --
