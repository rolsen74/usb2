
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
EthDevice_AbortIO( struct DeviceManagerInterface *Self, struct IORequest *request )
{
	struct USB2EthDevice *base;
	struct USB2EthUnit *unit;
	struct ExecIFace *IExec;

	base = (PTR) Self->Data.LibBase;
	IExec = base->USB->usb_IExec;

	IExec->ObtainSemaphore( & base->RouteLock );

	unit = (PTR) request->io_Unit;

	if ( ( unit ) && ( unit->Detached ) )
	{
		unit = NULL;
	}

	IExec->ReleaseSemaphore( & base->RouteLock );

	if ( ( unit ) && ( request->io_Message.mn_Length >= sizeof( struct IOSana2Req ) ) )
	{
		unit->Ops->Abort( unit->Adapter, (PTR) request );
	}
}

// --
