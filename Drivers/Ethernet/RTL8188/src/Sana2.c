
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Commands.h"
#include "Radio.h"

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

			case CMD_WRITE:
				pending = Cmd_Write( unit, ioreq );

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

			case S2_ADDMULTICASTADDRESS:
				Cmd_AddMultiCastAddress( unit, ioreq );

				break;

			case S2_DELMULTICASTADDRESS:
				Cmd_DelMultiCastAddress( unit, ioreq );

				break;

			case S2_MULTICAST:
				pending = Cmd_MultiCast( unit, ioreq );

				break;

			case S2_BROADCAST:
				pending = Cmd_BroadCast( unit, ioreq );

				break;

			case S2_GETSPECIALSTATS:
				Cmd_GetSpecialStats( unit, ioreq );

				break;

			case S2_GETGLOBALSTATS:
				Cmd_GetGlobalStats( unit, ioreq );

				break;

			case S2_ONEVENT:
				pending = Cmd_OnEvent( unit, ioreq );

				break;

			case S2_READORPHAN:
				pending = Cmd_ReadOrphan( unit, ioreq );

				break;

			case S2_ONLINE:
				Cmd_Online( unit, ioreq );

				break;

			case S2_OFFLINE:
				Cmd_Offline( unit, ioreq );

				break;

			case S2_GETEXTENDEDGLOBALSTATS:
				Cmd_GetExtendedGlobalStats( unit, ioreq );

				break;

			case S2_SAMPLE_THROUGHPUT:
				pending = Cmd_SampleThroughput( unit, ioreq );

				break;

			case S2_SANA2HOOK:
				Cmd_Sana2Hook( unit, ioreq );

				break;

			case S2_SETOPTIONS:
			case S2_SETKEY:
				RTL8188EU_QueueControl( unit, ioreq );

				pending = TRUE;

				break;

			case S2_READMGMT:
				pending = Cmd_ReadMgmt( unit, ioreq );

				break;

			case S2_WRITEMGMT:
				pending = Cmd_WriteMgmt( unit, ioreq );

				break;

			case S2_GETCRYPTTYPES:
				Cmd_GetCryptTypes( unit, ioreq );

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
