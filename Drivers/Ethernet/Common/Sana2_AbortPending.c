
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <interfaces/exec.h>
#include "WirelessSana2.h"
#include "Sana2Commands.h"

// --

extern struct ExecIFace *IExec;

// --

static int
PacketCommand( uint16 command )
{
	switch( command )
	{
		case CMD_READ:
		case CMD_WRITE:
		case S2_READORPHAN:
		case S2_BROADCAST:
		case S2_MULTICAST:
		case S2_READMGMT:
		case S2_WRITEMGMT:
			return ( TRUE );
	}

	return ( FALSE );
}

// --

static int32
AbortPending( struct List *list, struct IORequest *request, APTR opener, int32 error,
	uint32 wire, Sana2Reply reply, APTR context, int packets_only )
{
	struct IOSana2Req *ioreq;
	struct IOSana2Req *next;

	int32 aborted;

	aborted = 0;

	ioreq = (APTR) IExec->GetHead( list );

	while( ioreq )
	{
		next = (APTR) IExec->GetSucc( (APTR) ioreq );

		if ( ( ( ! request ) || ( & ioreq->ios2_Req == request ) ) &&
			( ( ! opener ) || ( ioreq->ios2_BufferManagement == opener ) ) &&
			( ( ! packets_only ) || ( PacketCommand( ioreq->ios2_Req.io_Command ) ) ) )
		{
			IExec->Remove( (APTR) ioreq );

			ioreq->ios2_Req.io_Error = error;
			ioreq->ios2_WireError = wire;

			if ( reply )
			{
				reply( context, ioreq );
			}
			else
			{
				IExec->ReplyMsg( (APTR) ioreq );
			}

			aborted++;

			if ( request )
			{
				break;
			}
		}

		ioreq = next;
	}

	return ( aborted );
}

// --

int32
Sana2_AbortPendingWithReply( struct List *list, struct IORequest *request, APTR opener, int32 error,
	uint32 wire, Sana2Reply reply, APTR context )
{
	return ( AbortPending( list, request, opener, error, wire, reply, context, FALSE ) );
}

// --

void
Sana2_AbortPackets( struct List *list, int32 error, uint32 wire )
{
	AbortPending( list, NULL, NULL, error, wire, NULL, NULL, TRUE );
}

// --

int32
Sana2_AbortPending( struct List *list, struct IORequest *request, APTR opener, int32 error, uint32 wire )
{
	return ( Sana2_AbortPendingWithReply( list, request, opener, error, wire, NULL, NULL ) );
}

// --
