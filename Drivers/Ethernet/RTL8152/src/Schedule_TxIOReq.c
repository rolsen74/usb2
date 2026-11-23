
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

void
Schedule_TxIOReq( struct devUnit *unit )
{
struct IOSana2Req *ioreq;
struct IONode *node;

	if ( ( ! unit->unit_Running ) || ( ! unit->unit_Online ) )
	{
		goto bailout;
	}

	while(( IExec->GetHead( & unit->unit_TxList )) && ( IExec->GetHead( & unit->unit_USBBulkOutFreeList )))
	{
		ioreq = (APTR) IExec->RemHead( & unit->unit_TxList );

		SHOWMSG( "TxList Node = %p", ioreq );

		node = (APTR) IExec->RemHead( & unit->unit_USBBulkOutFreeList );
		node->Sana2IOReq = ioreq;

		SHOWMSG( "Starting TxNode = %p", node );

		if ( SendPacket( unit, node ) )
		{
			IExec->AddTail( & unit->unit_USBBulkOutActiveList, & node->Node );
		}
		else
		{
			node->Sana2IOReq = NULL;

			ReplySanaRequest( unit, (PTR) ioreq );

			IExec->AddTail( & unit->unit_USBBulkOutFreeList, & node->Node );
		}
	}

bailout:

}

// --
