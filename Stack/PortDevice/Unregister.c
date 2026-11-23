
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
PortDevice_Unregister( struct USB2PortIFace *Self, struct USB2PortUnit *unit )
{
	struct USB2PortDevice *base;
	struct USB2PortUnit **link;
	struct ExecIFace *IExec;

	base = (PTR) Self->Data.LibBase;
	IExec = base->IExec;

	IExec->ObtainSemaphore( & base->Lock );

	link = & base->Units;

	while( ( *link ) && ( *link != unit ) )
	{
		link = & (*link)->Next;
	}

	if ( *link )
	{
		*link = unit->Next;
		unit->Next = NULL;
		unit->BeginPort = NULL;
		unit->AbortPort = NULL;

		PortDevice_ReplyClose( IExec, unit );

		// Existing opens keep their original unit after the number is reused.
		if ( ! unit->Opens )
		{
			IExec->FreeVec( unit );
		}
	}

	IExec->ReleaseSemaphore( & base->Lock );
}

// --
