
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <interfaces/exec.h>
#include "Sana2Commands.h"

// --

extern struct ExecIFace *IExec;

// --

struct TrackedType
{
	struct Node tt_Node;
	struct Sana2PacketTypeStats tt_Stats;
	uint32 tt_Type;
};

// --

static struct TrackedType *
FindType( struct List *list, uint32 type )
{
	struct TrackedType *entry;

	for( entry = (APTR) IExec->GetHead( list ); entry; entry = (APTR) IExec->GetSucc( & entry->tt_Node ) )
	{
		if ( entry->tt_Type == type )
		{
			return ( entry );
		}
	}

	return ( NULL );
}

// --

void
Sana2_TrackType( struct List *list, struct IOSana2Req *ioreq )
{
	struct TrackedType *entry;

	ioreq->ios2_WireError = 0;

	entry = FindType( list, ioreq->ios2_PacketType );

	if ( ioreq->ios2_Req.io_Command == S2_TRACKTYPE )
	{
		if ( entry )
		{
			ioreq->ios2_Req.io_Error = S2ERR_BAD_STATE;
			ioreq->ios2_WireError = S2WERR_ALREADY_TRACKED;

			return;
		}

		entry = IExec->AllocVecTags( sizeof( *entry ), AVT_ClearWithValue, 0, TAG_END );

		if ( ! entry )
		{
			ioreq->ios2_Req.io_Error = S2ERR_NO_RESOURCES;

			return;
		}

		entry->tt_Type = ioreq->ios2_PacketType;

		IExec->AddTail( list, & entry->tt_Node );

		return;
	}

	if ( ! entry )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_STATE;
		ioreq->ios2_WireError = S2WERR_NOT_TRACKED;

		return;
	}

	if ( ioreq->ios2_Req.io_Command == S2_UNTRACKTYPE )
	{
		IExec->Remove( & entry->tt_Node );

		IExec->FreeVec( entry );
	}
	else if ( ! ioreq->ios2_StatData )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_NULL_POINTER;
	}
	else
	{
		IExec->CopyMem( & entry->tt_Stats, ioreq->ios2_StatData, sizeof( entry->tt_Stats ) );
	}
}

// --

void
Sana2_AccountType( struct List *list, uint32 type, uint32 bytes, int receive, int dropped )
{
	struct TrackedType *entry;

	entry = FindType( list, type );

	if ( ! entry )
	{
		return;
	}

	if ( dropped )
	{
		entry->tt_Stats.PacketsDropped++;
	}
	else if ( receive )
	{
		entry->tt_Stats.PacketsReceived++;
		entry->tt_Stats.BytesReceived += bytes;
	}
	else
	{
		entry->tt_Stats.PacketsSent++;
		entry->tt_Stats.BytesSent += bytes;
	}
}

// --
