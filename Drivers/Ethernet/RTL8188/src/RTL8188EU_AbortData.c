
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Firmware.h"
#include "Radio.h"

// --

void
RTL8188EU_AbortData( struct EthernetUnit *unit, S32 error, U32 wire )
{
	struct IOSana2Req *request;
	struct IOSana2Req *next;

	request = (PTR) GetHead( & unit->unit_Pending );

	while( request )
	{
		next = (PTR) GetSucc( (PTR) request );

		switch( request->ios2_Req.io_Command )
		{
			case CMD_READ:
			case CMD_WRITE:
			case S2_READORPHAN:
			case S2_MULTICAST:
			case S2_BROADCAST:
				Remove( (PTR) request );

				request->ios2_Req.io_Error = error;
				request->ios2_WireError = wire;

				ReplyMsg( (PTR) request );

				break;
		}

		request = next;
	}
}

// --
