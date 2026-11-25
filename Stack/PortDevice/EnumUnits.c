
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "PortDevice.h"

// --

SEC_CODE void
PortDevice_EnumUnits( struct USB2PortDevice *base, struct IOStdReq *request )
{
	struct USB2PortUnit *unit;
	struct USB2UnitInfo entry;

	U8 *buffer;
	U32 count;

	request->io_Actual = 0;

	if ( request->io_Offset != USB2_UNIT_LIST_VERSION )
	{
		request->io_Error = IOERR_NOCMD;

		return;
	}

	base->IExec->ObtainSemaphore( & base->Lock );

	count = 0;

	for( unit = base->Units; unit; unit = unit->Next )
	{
		count += ( unit->BeginPort && ! unit->Quiescing );
	}

	request->io_Actual = count * sizeof( entry );

	if ( ( request->io_Actual > request->io_Length ) || ( ( count ) && ( ! request->io_Data ) ) )
	{
		request->io_Error = IOERR_BADLENGTH;
	}
	else
	{
		buffer = request->io_Data;

		for( unit = base->Units; unit; unit = unit->Next )
		{
			if ( ( ! unit->BeginPort ) || ( unit->Quiescing ) )
			{
				continue;
			}

			entry.Number = unit->Number;
			entry.Flags = ( ( unit->Opens ) || ( unit->Closing ) ) ? USB2_UNIT_BUSY : 0;

			base->IExec->CopyMem( & entry, buffer, sizeof( entry ) );

			buffer += sizeof( entry );
		}
	}

	base->IExec->ReleaseSemaphore( & base->Lock );
}

// --
