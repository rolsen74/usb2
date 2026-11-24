
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
	struct BufferManagement *opener;

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
				ioreq->ios2_Req.io_Error = IOERR_NOCMD;

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

			case S2_ADDMULTICASTADDRESS:
			case S2_DELMULTICASTADDRESS:
			case S2_ADDMULTICASTADDRESSES:
			case S2_DELMULTICASTADDRESSES:
				Sana2_MulticastRange( & unit->unit_MulticastRanges, ioreq );

				break;

			case S2_TRACKTYPE:
			case S2_UNTRACKTYPE:
			case S2_GETTYPESTATS:
				Sana2_TrackType( & unit->unit_TrackedTypes, ioreq );

				break;

			case S2_READORPHAN:
				pending = Cmd_ReadOrphan( unit, ioreq );

				break;

			case S2_GETSPECIALSTATS:
				Sana2_GetSpecialStats( ioreq, NULL );

				break;

			case S2_GETGLOBALSTATS:
				Sana2_GetPacketStats( ioreq, unit->unit_PacketsRx, unit->unit_PacketsTx, 0,
					unit->unit_UnknownPackets, & unit->unit_LastStart );

				break;

			case S2_GETEXTENDEDGLOBALSTATS:
				Sana2_GetExtendedPacketStats( ioreq, unit->unit_PacketsRx, unit->unit_PacketsTx, 0,
					unit->unit_UnknownPackets, FALSE, & unit->unit_LastStart );

				break;

			case S2_SAMPLE_THROUGHPUT:
				pending = Sana2_QueueThroughput( & unit->unit_Pending, ioreq, NULL, FALSE );

				break;

			case S2_GETCRYPTTYPES:
				Cmd_GetCryptTypes( unit, ioreq );

				break;

			case S2_ONEVENT:
				pending = Sana2_QueueEvent( & unit->unit_Pending, ioreq,
					unit->unit_Online ? S2EVENT_ONLINE : S2EVENT_OFFLINE, SANA2_COMMON_EVENTS );

				break;

			case S2_SANA2HOOK:
				opener = ioreq->ios2_BufferManagement;

				Sana2_SetCopyHook( ioreq, opener ? & opener->bm_CopyHook : NULL, FALSE );

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
				ioreq->ios2_Req.io_Error = IOERR_NOCMD;

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
