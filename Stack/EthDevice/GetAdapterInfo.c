
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "EthDevice.h"

// --

SEC_CODE LONG
EthDevice_GetAdapterInfo( struct USB2EthIFace *Self, const struct IORequest *request,
	struct USB2EthAdapterInfo *info )
{
	struct USB2EthDevice *base;
	struct USB2EthUnit *unit;
	struct ExecIFace *IExec;

	LONG found;

	base = (PTR) Self->Data.LibBase;

	if ( ( ! request ) || ( request->io_Device != (PTR) base ) || ( ! info ) )
	{
		return ( FALSE );
	}

	IExec = base->USB->usb_IExec;
	found = FALSE;

	IExec->ObtainSemaphore( & base->RouteLock );

	unit = (PTR) request->io_Unit;

	if ( ( unit ) && ( ! unit->Detached ) )
	{
		IExec->CopyMem( & unit->Info, info, sizeof( *info ) );

		found = TRUE;
	}

	IExec->ReleaseSemaphore( & base->RouteLock );

	return ( found );
}

// --
