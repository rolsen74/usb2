
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "PortDevice.h"

// --

SEC_CODE enum USB2SerialSession
PortDevice_SerialSession( struct USB2SerialIFace *Self, struct USB2PortUnit *unit )
{
	struct USB2PortDevice *base;
	struct ExecIFace *IExec;
	enum USB2SerialSession state;

	base = (PTR)Self->Data.LibBase;
	IExec = base->IExec;

	IExec->ObtainSemaphore( &base->Lock );

	state = unit->Closing ? USB2_SERIAL_CLOSING : ( unit->Opens ? USB2_SERIAL_OPEN : USB2_SERIAL_IDLE );

	IExec->ReleaseSemaphore( &base->Lock );

	return ( state );
}

// --

SEC_CODE void
PortDevice_SerialFinishClose( struct USB2SerialIFace *Self, struct USB2PortUnit *unit )
{
	struct USB2PortDevice *base;
	struct ExecIFace *IExec;

	base = (PTR)Self->Data.LibBase;
	IExec = base->IExec;

	IExec->ObtainSemaphore( &base->Lock );

	PortDevice_ReplyClose( IExec, unit );

	IExec->ReleaseSemaphore( &base->Lock );
}

// --
