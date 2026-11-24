
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

static int
OpenerMatched( struct List *list, const struct IOSana2Req *request )
{
	struct IOSana2Req *match;

	for( match = (APTR) IExec->GetHead( list ); match; match = (APTR) IExec->GetSucc( (APTR) match ) )
	{
		if ( match->ios2_BufferManagement == request->ios2_BufferManagement )
		{
			return ( TRUE );
		}
	}

	return ( FALSE );
}

// --

static const uint8 *
PrepareRead( struct IOSana2Req *request, const uint8 *frame, uint32 bytes, uint8 flags )
{
	uint32 offset;

	offset = 0;

	if ( ! ( request->ios2_Req.io_Flags & SANA2IOF_RAW ) )
	{
		offset = ( request->ios2_Req.io_Command == S2_READORPHAN ) ? 12 : 14;
	}

	request->ios2_Req.io_Flags = ( request->ios2_Req.io_Flags & ~( SANA2IOF_MCAST | SANA2IOF_BCAST ) ) | flags;
	request->ios2_DataLength = bytes - offset;

	IExec->CopyMem( frame, request->ios2_DstAddr, 6 );

	IExec->CopyMem( frame + 6, request->ios2_SrcAddr, 6 );

	return ( frame + offset );
}

// --

uint32
Sana2_DeliverReads( struct List *list, const uint8 *frame, uint32 bytes, uint32 type,
	uint8 flags, int allow_orphan, int length_types, Sana2ReceiveFilter filter,
	Sana2ReceiveCopy copy, Sana2Reply reply, APTR context )
{
	struct List matches;
	struct IOSana2Req *request;
	struct IOSana2Req *next;
	struct IOSana2Req *orphan;

	const uint8 *payload;
	uint32 result;
	uint32 old_length;
	uint8 old_flags;

	result = 0;
	orphan = NULL;

	if ( bytes < 14 )
	{
		return ( result );
	}

	IExec->NewList( & matches );

	for( request = (APTR) IExec->GetHead( list ); request; request = next )
	{
		next = (APTR) IExec->GetSucc( (APTR) request );

		if ( ( allow_orphan ) && ( ! orphan ) && ( request->ios2_Req.io_Command == S2_READORPHAN ) )
		{
			orphan = request;
		}

		if ( ( request->ios2_Req.io_Command != CMD_READ ) ||
			( ( request->ios2_PacketType != type ) &&
			  ( ( ! length_types ) || ( type > 1500 ) || ( request->ios2_PacketType > 1500 ) ) ) ||
			( OpenerMatched( & matches, request ) ) )
		{
			continue;
		}

		if ( filter )
		{
			old_length = request->ios2_DataLength;
			old_flags = request->ios2_Req.io_Flags;

			payload = PrepareRead( request, frame, bytes, flags );

			if ( ! filter( request, payload ) )
			{
				request->ios2_DataLength = old_length;
				request->ios2_Req.io_Flags = old_flags;

				continue;
			}
		}

		IExec->Remove( (APTR) request );

		IExec->AddTail( & matches, (APTR) request );

		result |= SANA2_RX_TYPED;
	}

	if ( ( ! ( result & SANA2_RX_TYPED ) ) && ( orphan ) )
	{
		IExec->Remove( (APTR) orphan );

		IExec->AddTail( & matches, (APTR) orphan );
	}

	while(( request = (APTR) IExec->RemHead( & matches ) ))
	{
		payload = PrepareRead( request, frame, bytes, flags );
		request->ios2_PacketType = type;
		request->ios2_WireError = 0;

		if ( copy( context, request, payload ) )
		{
			result |= SANA2_RX_COPIED;
		}
		else
		{
			request->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
			request->ios2_WireError = S2WERR_BUFF_ERROR;
			request->ios2_DataLength = 0;
			result |= SANA2_RX_COPY_FAILED;
		}

		if ( reply )
		{
			reply( context, request );
		}
		else
		{
			IExec->ReplyMsg( (APTR) request );
		}
	}

	return ( result );
}

// --
