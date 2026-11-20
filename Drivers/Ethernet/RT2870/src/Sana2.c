
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Commands.h"
#include "WiFiCommand.h"

// --

void
Sana2BeginIO( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	S32 pending;

	pending = FALSE;
	ioreq->ios2_Req.io_Error = 0;

	if ( ( ioreq->ios2_Req.io_Command != S2_ONEVENT ) && ( ioreq->ios2_Req.io_Command != S2_SETKEY ) )
	{
		ioreq->ios2_WireError = 0;
	}

	ObtainSemaphore( & unit->unit_CommandLock );

	if ( ( unit->unit_Detached ) && ( ioreq->ios2_Req.io_Command != CMD_INVALID ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
		ioreq->ios2_WireError = S2WERR_UNIT_DISCONNECTED;
	}
	else
	{
		switch( ioreq->ios2_Req.io_Command )
		{
			case CMD_INVALID:
				Cmd_Invalid( unit, ioreq );

				break;

			case CMD_READ:
				pending = Cmd_Read( unit, ioreq );

				break;

			case CMD_FLUSH:
				Cmd_Flush( unit, ioreq );

				break;

			case S2_DEVICEQUERY:
				Cmd_DeviceQuery( unit, ioreq );

				break;

			case S2_GETSTATIONADDRESS:
				Cmd_GetStationAddress( unit, ioreq );

				break;

			case S2_CONFIGINTERFACE:
				Cmd_ConfigInterface( unit, ioreq );

				break;

			case S2_ONEVENT:
				pending = Cmd_OnEvent( unit, ioreq );

				break;

			case S2_SANA2HOOK:
				Cmd_Sana2Hook( unit, ioreq );

				break;

			case S2_ONLINE:
			case S2_OFFLINE:
			case S2_BROADCAST:
			case S2_MULTICAST:
			case CMD_WRITE:
			case USB2WIRELESS_CMD_PROMPT:
			case S2_SETKEY:
			case S2_WRITEMGMT:
			case S2_SETOPTIONS:
				AddTail( & unit->unit_Pending, (PTR) ioreq );

				Signal( unit->unit_TaskAdr, SIGBREAKF_CTRL_E );

				pending = TRUE;

				break;

			case S2_READMGMT:
				pending = Cmd_ReadMgmt( unit, ioreq );

				break;

			default:
				Cmd_Invalid( unit, ioreq );

				break;
		}
	}

	ReleaseSemaphore( & unit->unit_CommandLock );

	if ( ! pending )
	{
		ReplyMsg( (PTR) ioreq );
	}
}

// --
