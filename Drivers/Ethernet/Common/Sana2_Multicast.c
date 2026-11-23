
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

struct MulticastRange
{
	struct Node mr_Node;
	uint64 mr_First;
	uint64 mr_Last;
	uint32 mr_UseCount;
};

// --

static uint64
AddressValue( const uint8 *address )
{
	uint64 value;
	uint32 index;

	value = 0;

	for( index = 0; index < 6; index++ )
	{
		value = ( value << 8 ) | address[index];
	}

	return ( value );
}

// --

int
Sana2_MulticastMatch( struct List *list, const uint8 *address )
{
	struct MulticastRange *entry;

	uint64 value;

	value = AddressValue( address );

	for( entry = (APTR) IExec->GetHead( list ); entry; entry = (APTR) IExec->GetSucc( & entry->mr_Node ) )
	{
		if ( ( value >= entry->mr_First ) && ( value <= entry->mr_Last ) )
		{
			return ( TRUE );
		}
	}

	return ( FALSE );
}

// --

static void
SetMulticast( struct List *list, struct IOSana2Req *ioreq, uint32 full_error )
{
	struct MulticastRange *entry;

	uint64 first;
	uint64 last;
	uint16 command;

	command = ioreq->ios2_Req.io_Command;
	ioreq->ios2_WireError = 0;

	first = AddressValue( ioreq->ios2_SrcAddr );

	last = first;

	if ( ( command == S2_ADDMULTICASTADDRESSES ) || ( command == S2_DELMULTICASTADDRESSES ) )
	{
		last = AddressValue( ioreq->ios2_DstAddr );
	}

	if ( ( ! ( ioreq->ios2_SrcAddr[0] & 1 ) ) || ( first > last ) ||
		( ( first >> 40 ) != ( last >> 40 ) ) || ( last == 0xffffffffffffULL ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ADDRESS;
		ioreq->ios2_WireError = S2WERR_BAD_MULTICAST;

		return;
	}

	for( entry = (APTR) IExec->GetHead( list ); entry; entry = (APTR) IExec->GetSucc( & entry->mr_Node ) )
	{
		if ( ( entry->mr_First == first ) && ( entry->mr_Last == last ) )
		{
			break;
		}
	}

	if ( ( command == S2_DELMULTICASTADDRESS ) || ( command == S2_DELMULTICASTADDRESSES ) )
	{
		if ( ! entry )
		{
			ioreq->ios2_Req.io_Error = S2ERR_BAD_ADDRESS;
			ioreq->ios2_WireError = S2WERR_BAD_MULTICAST;
		}
		else if ( --entry->mr_UseCount == 0 )
		{
			IExec->Remove( & entry->mr_Node );

			IExec->FreeVec( entry );
		}

		return;
	}

	if ( ! entry )
	{
		entry = IExec->AllocVecTags( sizeof( *entry ), AVT_ClearWithValue, 0, TAG_END );

		if ( entry )
		{
			entry->mr_First = first;
			entry->mr_Last = last;

			IExec->AddTail( list, & entry->mr_Node );
		}
	}

	if ( ( ! entry ) || ( entry->mr_UseCount == (uint32) -1 ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
		ioreq->ios2_WireError = full_error;

		return;
	}

	entry->mr_UseCount++;
}

// --

int
Sana2_ValidMulticastAddress( const uint8 *address )
{
	return ( ( address[0] & 1 ) && ( AddressValue( address ) != 0xffffffffffffULL ) );
}

// --

void
Sana2_MulticastAddress( struct List *list, struct IOSana2Req *ioreq, uint32 full_error )
{
	SetMulticast( list, ioreq, full_error );
}

// --

void
Sana2_MulticastRange( struct List *list, struct IOSana2Req *ioreq )
{
	SetMulticast( list, ioreq, S2WERR_MULTICAST_FULL );
}

// --
