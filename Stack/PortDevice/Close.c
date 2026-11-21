
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "PortDevice.h"

// --

SEC_CODE PTR
PortDevice_Close( struct DeviceManagerInterface *Self, struct IORequest *request )
{
	struct USB2PortDevice *base;
	struct USB2PortUnit *unit;
	struct ExecIFace *IExec;

	base = (PTR) Self->Data.LibBase;
	IExec = base->IExec;
	unit = (PTR) request->io_Unit;

	IExec->ObtainSemaphore( & base->Lock );

	base->Library.lib_OpenCnt--;
	request->io_Unit = NULL;
	request->io_Device = NULL;

	if ( unit )
	{
		unit->Opens--;

		if ( ( ! unit->BeginPort ) && ( ! unit->Opens ) )
		{
			IExec->FreeVec( unit );
		}
	}

	IExec->ReleaseSemaphore( & base->Lock );

	return ( NULL );
}

// --
