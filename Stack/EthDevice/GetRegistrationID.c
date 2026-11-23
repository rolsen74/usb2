
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
EthDevice_GetRegistrationID( struct USB2EthIFace *Self, const struct IORequest *request, ULONG number )
{
	struct USB2EthDevice *base;
	struct USB2EthUnit *unit;
	struct ExecIFace *IExec;

	ULONG registration;

	base = (PTR) Self->Data.LibBase;
	IExec = base->USB->usb_IExec;
	unit = NULL;
	registration = 0;

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
		registration = unit->RegistrationID;
	}

	IExec->ReleaseSemaphore( & base->RouteLock );

	return ( registration );
}

// --
