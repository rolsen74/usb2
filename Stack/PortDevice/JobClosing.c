
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "PortDevice.h"

// --

SEC_CODE BOOL
PortDevice_JobClosing( struct USB2PrinterIFace *Self, struct USB2PortUnit *unit )
{
	struct USB2PortDevice *base;
	struct ExecIFace *IExec;

	BOOL closing;

	base = (PTR) Self->Data.LibBase;
	IExec = base->IExec;

	IExec->ObtainSemaphore( & base->Lock );

	closing = unit->Closing;

	IExec->ReleaseSemaphore( & base->Lock );

	return ( closing );
}

// --
