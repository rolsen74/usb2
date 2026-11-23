
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
	destroy = FALSE;

	IExec->ObtainSemaphore( & base->OpenLock );

	IExec->ObtainSemaphore( & base->RouteLock );

	if ( request->io_Device == (PTR) base )
	{
		unit = (PTR) request->io_Unit;
		base->Library.lib_OpenCnt--;
		request->io_Unit = NULL;
		request->io_Device = NULL;

		if ( unit )
		{
			destroy = ( ! --unit->Opens ) && ( unit->Detached );
		}
	}

	IExec->ReleaseSemaphore( & base->RouteLock );

	if ( unit )
	{
		unit->Ops->Close( unit->Adapter, (PTR) request );

		if ( destroy )
		{
			unit->Ops->Destroy( unit->Adapter );

			IExec->FreeVec( unit );
		}
	}

	IExec->ReleaseSemaphore( & base->OpenLock );

	return ( NULL );
}

// --
