
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

S32
RT2870_DeliverEAPOL( struct EthernetUnit *unit )
{
	struct IOSana2Req *request;

	if ( ! unit->unit_EAPOLLength )
	{
		return ( FALSE );
	}

	ObtainSemaphore( & unit->unit_CommandLock );

	request = (PTR) GetHead( & unit->unit_Pending );

	while( request )
	{
		if ( ( request->ios2_Req.io_Command == CMD_READ ) && ( request->ios2_PacketType == RT2870_EAPOL_TYPE ) )
		{
			Remove( (PTR) request );

			if ( request->ios2_DataLength < unit->unit_EAPOLLength )
			{
				request->ios2_Req.io_Error = IOERR_BADLENGTH;
				request->ios2_DataLength = 0;
			}
			else
			{
				memcpy( request->ios2_Data, unit->unit_EAPOL, unit->unit_EAPOLLength );

				memcpy( request->ios2_SrcAddr, unit->unit_Peer, sizeof( unit->unit_Peer ) );

				memcpy( request->ios2_DstAddr, unit->unit_StationAddress, sizeof( unit->unit_StationAddress ) );

				request->ios2_DataLength = unit->unit_EAPOLLength;
			}

			unit->unit_EAPOLLength = 0;

			ReplyMsg( (PTR) request );

			break;
		}

		request = (PTR) GetSucc( (PTR) request );
	}

	ReleaseSemaphore( & unit->unit_CommandLock );

	return ( request != NULL );
}

// --
