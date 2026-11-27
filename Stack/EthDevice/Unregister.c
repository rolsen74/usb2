
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
EthDevice_DestroyUnit( struct ExecIFace *IExec, struct USB2EthUnit *unit )
{
	unit->Ops->Destroy( unit->Adapter );

	IExec->FreeVec( unit );
}

// --

SEC_CODE void
EthDevice_Unregister( struct USB2EthIFace *Self, struct USB2EthUnit *unit )
{
	struct USB2EthDevice *base;
	struct ExecIFace *IExec;

	S32 destroy;

	base = (PTR) Self->Data.LibBase;
	IExec = base->USB->usb_IExec;

	IExec->ObtainSemaphore( & base->OpenLock );

	IExec->ObtainSemaphore( & base->RouteLock );

	base->Units[unit->Number] = NULL;
	base->Reserved[unit->Number] += unit->Opens + unit->Opening;
	unit->Detached = TRUE;

	destroy = ( ! unit->Opens ) && ( ! unit->Opening );

	IExec->ReleaseSemaphore( & base->RouteLock );

	IExec->ReleaseSemaphore( & base->OpenLock );

	if ( destroy )
	{
		EthDevice_DestroyUnit( IExec, unit );
	}
}

// --
