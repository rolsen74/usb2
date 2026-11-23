
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "EthDevice.h"

// --

SEC_CODE ULONG
EthDevice_GetAdapter( struct USB2EthIFace *Self, const struct IORequest *request, ULONG number )
{
	struct USB2EthDevice *base;
	struct USB2EthUnit *unit;
	struct ExecIFace *IExec;

	ULONG type;

	base = (PTR) Self->Data.LibBase;
	IExec = base->USB->usb_IExec;
	unit = NULL;
	type = USB2ETH_ADAPTER_UNKNOWN;

	IExec->ObtainSemaphore( & base->RouteLock );

	if ( request )
	{
		if ( request->io_Device == (PTR) base )
		{
			unit = (PTR) request->io_Unit;
		}
	}
	else if ( number < USB2_ETH_MAX_UNITS )
	{
		unit = base->Units[number];
	}

	if ( ( unit ) && ( ! unit->Detached ) )
	{
		type = unit->Type;
	}

	IExec->ReleaseSemaphore( & base->RouteLock );

	return ( type );
}

// --
