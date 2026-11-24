
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "PortDevice.h"

// --

SEC_CODE static struct USB2PortUnit *
Register( struct USB2PortDevice *base, struct MsgPort *begin, struct MsgPort *abort, ULONG *number, BOOL sessions )
{
	struct USB2PortUnit **link;
	struct USB2PortUnit *unit;
	struct ExecIFace *IExec;

	U32 index;

	// Exec stores allocation flags alongside the arrival action.
	if ( ( ! begin ) || ( ! abort ) || ( ! number ) ||
		( ( begin->mp_Flags & PF_ACTION ) != PA_SIGNAL ) ||
		( ( abort->mp_Flags & PF_ACTION ) != PA_SIGNAL ) ||
		( begin == abort ) || ( ! begin->mp_SigTask ) ||
		( begin->mp_SigTask != abort->mp_SigTask ) ||
		( begin->mp_SigBit == abort->mp_SigBit ) ||
		( begin->mp_SigBit >= 32 ) || ( abort->mp_SigBit >= 32 ) )
	{
		return ( NULL );
	}

	IExec = base->IExec;

	unit = IExec->AllocVecTags( sizeof( *unit ), AVT_Clear, 0, TAG_END );

	if ( ! unit )
	{
		return ( NULL );
	}

	unit->BeginPort = begin;
	unit->AbortPort = abort;
	unit->SerialSessions = sessions;
	index = 0;

	IExec->ObtainSemaphore( & base->Lock );

	link = & base->Units;

	while( ( *link ) && ( (*link)->Number == index ) )
	{
		link = & (*link)->Next;
		index++;
	}

	if ( index == USB2_PORT_BACKEND_UNIT )
	{
		IExec->ReleaseSemaphore( & base->Lock );

		IExec->FreeVec( unit );

		return ( NULL );
	}

	unit->Number = index;
	unit->Next = *link;
	*link = unit;
	*number = index;

	IExec->ReleaseSemaphore( & base->Lock );

	return ( unit );
}

// --

SEC_CODE struct USB2PortUnit *
PortDevice_Register( struct USB2PortIFace *Self, struct MsgPort *begin, struct MsgPort *abort, ULONG *number )
{
	return ( Register( (PTR)Self->Data.LibBase, begin, abort, number, FALSE ) );
}

// --

SEC_CODE struct USB2PortUnit *
PortDevice_RegisterSession( struct USB2SerialIFace *Self, struct MsgPort *begin, struct MsgPort *abort, ULONG *number )
{
	return ( Register( (PTR)Self->Data.LibBase, begin, abort, number, TRUE ) );
}

// --
