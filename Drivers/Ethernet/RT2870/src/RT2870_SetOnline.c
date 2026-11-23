
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
RT2870_SetOnline( struct EthernetUnit *unit, S32 online )
{
	struct IOSana2Req *request;
	struct IOSana2Req *next;

	S32 changed;

	ObtainSemaphore( & unit->unit_CommandLock );

	changed = unit->unit_Online != online;
	unit->unit_Online = online;

	if ( changed )
	{
		Sana2_SignalEvent( & unit->unit_Pending, online ? S2EVENT_ONLINE : S2EVENT_OFFLINE, FALSE );
	}

	if ( ! online )
	{
		request = (PTR) GetHead( & unit->unit_Pending );

		while( request )
		{
			next = (PTR) GetSucc( (PTR) request );

			if ( ( request->ios2_Req.io_Command == S2_READORPHAN ) ||
				( ( request->ios2_Req.io_Command == CMD_READ ) &&
				  ( ( ! unit->unit_Operational ) || ( request->ios2_PacketType != RT2870_EAPOL_TYPE ) ) ) )
			{
				Remove( (PTR) request );

				request->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
				request->ios2_WireError = S2WERR_UNIT_OFFLINE;
				request->ios2_DataLength = 0;

				ReplyMsg( (PTR) request );
			}

			request = next;
		}
	}

	ReleaseSemaphore( & unit->unit_CommandLock );
}

// --
