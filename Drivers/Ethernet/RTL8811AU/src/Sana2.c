
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

S32
Sana2BeginIO( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	static const U8 crypt_types[] = { S2ENC_CCMP };

	struct BufferManagement *opener;

	opener = FindOpener( unit, ioreq->ios2_BufferManagement );

	if ( ! opener )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_NULL_POINTER;

		return ( FALSE );
	}

	switch( ioreq->ios2_Req.io_Command )
	{
		case CMD_READ:
		case S2_READORPHAN:
			return ( Cmd_Read( unit, opener, ioreq ) );

		case CMD_WRITE:
		case S2_MULTICAST:
		case S2_BROADCAST:
			return ( Cmd_Write( unit, opener, ioreq ) );

		case S2_WRITEMGMT:
			return ( Cmd_WriteMgmt( unit, ioreq ) );

		case S2_SETOPTIONS:
		case S2_SETKEY:
		case USB2WIRELESS_CMD_PROMPT:
			AddTail( & unit->unit_Pending, (PTR) ioreq );

			return ( TRUE );

		case CMD_FLUSH:
			Sana2_AbortPending( & unit->unit_Pending, NULL, NULL, IOERR_ABORTED, 0 );

			break;

		case S2_DEVICEQUERY:
			Cmd_DeviceQuery( ioreq );

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
			Sana2_MulticastRange( & opener->bm_Multicast, ioreq );

			break;

		case S2_TRACKTYPE:
		case S2_UNTRACKTYPE:
		case S2_GETTYPESTATS:
			Sana2_TrackType( & opener->bm_TrackedTypes, ioreq );

			break;

		case S2_GETSPECIALSTATS:
			Sana2_GetSpecialStats( ioreq, NULL );

			break;

		case S2_GETGLOBALSTATS:
			Sana2_GetPacketStats( ioreq, unit->unit_PacketsRx, unit->unit_PacketsTx,
				0, unit->unit_UnknownPackets, & unit->unit_LastStart );

			break;

		case S2_GETEXTENDEDGLOBALSTATS:
			Sana2_GetExtendedPacketStats( ioreq, unit->unit_PacketsRx, unit->unit_PacketsTx,
				0, unit->unit_UnknownPackets, FALSE, & unit->unit_LastStart );

			break;

		case S2_ONEVENT:
			return ( Sana2_QueueEvent( & unit->unit_Pending, ioreq,
				unit->unit_Online ? S2EVENT_ONLINE : S2EVENT_OFFLINE, SANA2_COMMON_EVENTS ) );

		case S2_SAMPLE_THROUGHPUT:
			return ( Sana2_QueueThroughput( & unit->unit_Pending, ioreq, NULL, FALSE ) );

		case S2_READMGMT:
			return ( Cmd_ReadMgmt( unit, ioreq ) );

		case S2_SANA2HOOK:
			Sana2_SetCopyHook( ioreq, & opener->bm_CopyHook, FALSE );

			break;

		case S2_GETCRYPTTYPES:
			Sana2_GetCryptTypes( ioreq, crypt_types, sizeof( crypt_types ) );

			break;

		case S2_ONLINE:
			Cmd_Online( unit, ioreq );

			break;

		case S2_OFFLINE:
			unit->unit_ManualOffline = TRUE;

			RTL8811AU_SetOnline( unit, FALSE );

			Sana2_AbortPackets( & unit->unit_Pending, S2ERR_OUTOFSERVICE, S2WERR_UNIT_OFFLINE );

			break;

		case S2_GETPEERADDRESS:
		case S2_GETDNSADDRESS:
		case S2_CONNECT:
		case S2_DISCONNECT:
		case S2_GETSIGNALQUALITY:
		case S2_GETNETWORKS:
		case S2_GETNETWORKINFO:
		case S2_GETRADIOBANDS:
			ioreq->ios2_Req.io_Error = S2ERR_NOT_SUPPORTED;

			break;

		default:
			ioreq->ios2_Req.io_Error = IOERR_NOCMD;

			break;
	}

	return ( FALSE );
}

// --
