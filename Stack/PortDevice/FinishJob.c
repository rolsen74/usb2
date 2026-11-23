
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
PortDevice_ReplyClose( struct ExecIFace *IExec, struct USB2PortUnit *unit )
{
	struct Message *reply;

	reply = unit->CloseReply;
	unit->CloseReply = NULL;
	unit->Closing = FALSE;

	if ( reply )
	{
		IExec->ReplyMsg( reply );
	}
}

// --

SEC_CODE void
PortDevice_FinishJob( struct USB2PrinterIFace *Self, struct USB2PortUnit *unit )
{
	struct USB2PortDevice *base;
	struct ExecIFace *IExec;

	base = (PTR) Self->Data.LibBase;
	IExec = base->IExec;

	IExec->ObtainSemaphore( & base->Lock );

	PortDevice_ReplyClose( IExec, unit );

	IExec->ReleaseSemaphore( & base->Lock );
}

// --
