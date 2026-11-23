
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
EthDevice_Unregister( struct USB2EthIFace *Self, struct USB2EthUnit *unit )
{
	struct USB2EthDevice *base;
	struct ExecIFace *IExec;

	base = (PTR) Self->Data.LibBase;
	IExec = base->USB->usb_IExec;

	IExec->ObtainSemaphore( & base->OpenLock );

	IExec->ObtainSemaphore( & base->RouteLock );

	base->Units[unit->Number] = NULL;
	unit->Detached = TRUE;

	if ( ! unit->Opens )
	{
		unit->Ops->Destroy( unit->Adapter );

		IExec->FreeVec( unit );
	}

	IExec->ReleaseSemaphore( & base->RouteLock );

	IExec->ReleaseSemaphore( & base->OpenLock );
}

// --
