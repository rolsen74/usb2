
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "PortDevice.h"

// --

SEC_CODE void
PortDevice_Quiesce( struct USB2PrinterIFace *Self, struct USB2PortUnit *unit )
{
	struct USB2PortDevice *base;
	struct ExecIFace *IExec;

	base = (PTR)Self->Data.LibBase;
	IExec = base->IExec;

	IExec->ObtainSemaphore( &base->Lock );

	unit->Quiescing = TRUE;

	IExec->ReleaseSemaphore( &base->Lock );
}

// --
