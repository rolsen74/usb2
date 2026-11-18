
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
Schedule_RxIOReq( struct devUnit *unit )
{
struct USB2_IORequest *usbio;
struct IONode *node;

	if ( ( ! unit->unit_Running ) || ( ! unit->unit_Online ) )
	{
		goto bailout;
	}

	while(( node = (APTR) IExec->RemHead( & unit->unit_USBBulkInFreeList )))
	{
		SHOWMSG( "Starting RxNode = %p", node );

		usbio = node->USBRequest;

		usbio->io_Command	= CMD_READ;
		usbio->io_Data		= node->Buffer;
		usbio->io_Length	= node->BufferSize;
		node->Active = TRUE;

		IExec->SendIO( (APTR) usbio );

		IExec->AddTail( & unit->unit_USBBulkInActiveList, & node->Node );
	}

bailout:

}

// --
