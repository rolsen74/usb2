
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
PortDevice_AbortIO( struct DeviceManagerInterface *Self, struct IOStdReq *request )
{
	struct USB2PortDevice *base;
	struct USB2PortUnit *unit;
	struct ExecIFace *IExec;

	base = (PTR) Self->Data.LibBase;
	IExec = base->IExec;
	unit = (PTR) request->io_Unit;

	IExec->ObtainSemaphore( & base->Lock );

	if ( ( unit ) && ( unit->AbortPort ) && ( request->io_Message.mn_Node.ln_Type == NT_MESSAGE ) )
	{
		request->io_Flags |= ( base->Type == PORT_SERIAL ) ? IOSERF_ABORT : IOPARF_ABORT;

		IExec->Signal( unit->AbortPort->mp_SigTask, 1UL << unit->AbortPort->mp_SigBit );
	}

	IExec->ReleaseSemaphore( & base->Lock );
}

// --
