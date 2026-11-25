
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "EthDevice.h"

// --

SEC_CODE void
EthDevice_EnumAdapters( struct USB2EthDevice *base, struct IOStdReq *request )
{
	struct USB2EthUnit *unit;
	struct USB2EthAdapterEntry entry;
	struct ExecIFace *IExec;

	U8 *output;
	U32 count;
	U32 index;

	request->io_Actual = 0;

	if ( request->io_Offset != USB2_ETH_ADAPTER_LIST_VERSION )
	{
		request->io_Error = IOERR_NOCMD;

		return;
	}

	IExec = base->USB->usb_IExec;

	IExec->ObtainSemaphore( & base->RouteLock );

	count = 0;

	for( index = 0; index < USB2_ETH_MAX_UNITS; index++ )
	{
		count += ( base->Units[index] != NULL );
	}

	request->io_Actual = count * sizeof( entry );
	request->io_Error = 0;

	if ( ( request->io_Actual > request->io_Length ) || ( ( count ) && ( ! request->io_Data ) ) )
	{
		request->io_Error = IOERR_BADLENGTH;
	}
	else
	{
		output = request->io_Data;

		for( index = 0; index < USB2_ETH_MAX_UNITS; index++ )
		{
			unit = base->Units[index];

			if ( ! unit )
			{
				continue;
			}

			entry.Number = index;

			IExec->CopyMem( & unit->Info, & entry.Info, sizeof( entry.Info ) );

			IExec->CopyMem( & entry, output, sizeof( entry ) );

			output += sizeof( entry );
		}
	}

	IExec->ReleaseSemaphore( & base->RouteLock );
}

// --
