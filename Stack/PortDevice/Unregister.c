
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "PortDevice.h"

// --

// Caller holds the device lock.
SEC_CODE void
PortDevice_UnlinkUnit( struct USB2PortDevice *base, struct USB2PortUnit *unit )
{
	struct USB2PortUnit **link;

	link = & base->Units;

	while( ( *link ) && ( *link != unit ) )
	{
		link = & (*link)->Next;
	}

	if ( *link )
	{
		*link = unit->Next;
		unit->Next = NULL;
	}
}

// --

SEC_CODE void
PortDevice_Unregister( struct USB2PortIFace *Self, struct USB2PortUnit *unit )
{
	struct USB2PortDevice *base;
	struct ExecIFace *IExec;

	base = (PTR) Self->Data.LibBase;
	IExec = base->IExec;

	IExec->ObtainSemaphore( &base->Lock );

	if ( unit->BeginPort )
	{
		unit->BeginPort = NULL;
		unit->AbortPort = NULL;

		PortDevice_ReplyClose( IExec, unit );

		if ( ! unit->Opens )
		{
			PortDevice_UnlinkUnit( base, unit );
		}

		if ( ! unit->Opens )
		{
			IExec->FreeVec( unit );
		}
	}

	IExec->ReleaseSemaphore( & base->Lock );
}

// --
