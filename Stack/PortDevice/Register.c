
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
Register( struct USB2PortDevice *base, struct MsgPort *begin, struct MsgPort *abort, ULONG *number, BOOL sessions,
	const struct USB2_DriverMessage *msg )
{
	struct USB2PortUnit **link;
	struct USB2PortUnit *unit;
	struct USB2PortUnit *slot;
	struct USB2PortUnit *matching;
	struct ExecIFace *IExec;

	U32 index;
	S32 match;
	S32 best;

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
	matching = NULL;
	best = 0;
	index = 0;

	USB2_ReadUnitIdentity( &unit->Identity, msg, 0 );

	IExec->ObtainSemaphore( & base->Lock );

	for ( slot = base->Units; slot; slot = slot->Next )
	{
		if ( ( ! slot->BeginPort ) && ( slot->SerialSessions == sessions ) )
		{
			match = USB2_MatchUnitIdentity( &slot->Identity, &unit->Identity );

			if ( match > best )
			{
				best = match;
				matching = slot;
			}
		}
	}

	if ( matching )
	{
		matching->BeginPort = begin;
		matching->AbortPort = abort;
		matching->Quiescing = FALSE;
		*number = matching->Number;

		IExec->CopyMem( &unit->Identity, &matching->Identity, sizeof( unit->Identity ) );

		if ( ( sessions ) && ( matching->Opens ) )
		{
			IExec->Signal( abort->mp_SigTask, 1UL << abort->mp_SigBit );
		}

		IExec->ReleaseSemaphore( &base->Lock );

		IExec->FreeVec( unit );

		return ( matching );
	}

	link = & base->Units;

	while( ( *link ) && ( (*link)->Number == index ) )
	{
		link = & (*link)->Next;
		index++;
	}

	if ( index == USB2_DEFAULT_UNIT )
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
	return ( Register( (PTR)Self->Data.LibBase, begin, abort, number, FALSE, NULL ) );
}

// --

SEC_CODE struct USB2PortUnit *
PortDevice_RegisterSession( struct USB2SerialIFace *Self, struct MsgPort *begin, struct MsgPort *abort, ULONG *number )
{
	return ( Register( (PTR)Self->Data.LibBase, begin, abort, number, TRUE, NULL ) );
}

// --

SEC_CODE struct USB2PortUnit *
PortDevice_RegisterDevice( struct USB2PortIFace *Self, struct MsgPort *begin, struct MsgPort *abort, ULONG *number,
	const struct USB2_DriverMessage *msg )
{
	struct USB2PortDevice *base;

	base = (PTR)Self->Data.LibBase;

	return ( Register( base, begin, abort, number, base->Type == PORT_SERIAL, msg ) );
}

// --
