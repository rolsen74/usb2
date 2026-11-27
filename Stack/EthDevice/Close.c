
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "EthDevice.h"

// --

SEC_CODE PTR
EthDevice_Close( struct DeviceManagerInterface *Self, struct IORequest *request )
{
	struct USB2EthDevice *base;
	struct USB2EthUnit *unit;
	struct ExecIFace *IExec;

	S32 destroy;

	base = (PTR) Self->Data.LibBase;
	IExec = base->USB->usb_IExec;
	unit = NULL;

	IExec->ObtainSemaphore( & base->OpenLock );

	IExec->ObtainSemaphore( & base->RouteLock );

	if ( request->io_Device == (PTR) base )
	{
		unit = (PTR) request->io_Unit;
		base->Library.lib_OpenCnt--;
		request->io_Unit = NULL;
		request->io_Device = NULL;
	}

	IExec->ReleaseSemaphore( & base->RouteLock );

	IExec->ReleaseSemaphore( & base->OpenLock );

	if ( unit )
	{
		unit->Ops->Close( unit->Adapter, (PTR) request );

		IExec->ObtainSemaphore( & base->OpenLock );

		if ( unit->Detached )
		{
			base->Reserved[unit->Number]--;
		}

		// Keep the last open reference until the adapter callback returns.
		destroy = ( ! --unit->Opens ) && ( ! unit->Opening ) && ( unit->Detached );

		IExec->ReleaseSemaphore( & base->OpenLock );

		if ( destroy )
		{
			EthDevice_DestroyUnit( IExec, unit );
		}
	}

	return ( NULL );
}

// --
