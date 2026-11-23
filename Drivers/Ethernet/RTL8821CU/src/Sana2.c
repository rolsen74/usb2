
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Commands.h"

// --

S32
Sana2BeginIO( struct EthernetUnit *unit, struct IOSana2Req *sana )
{
	struct BufferManagement *opener;

	if ( sana->ios2_Req.io_Command != S2_ONEVENT )
	{
		sana->ios2_WireError = 0;
	}

	opener = FindOpener( unit, sana->ios2_BufferManagement );

	if ( ! opener )
	{
		sana->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		sana->ios2_WireError = S2WERR_NULL_POINTER;

		return ( FALSE );
	}

	switch( sana->ios2_Req.io_Command )
	{
		case CMD_READ:
		case S2_READORPHAN:
			return ( Cmd_Read( unit, opener, sana ) );

		case CMD_WRITE:
		case S2_MULTICAST:
		case S2_BROADCAST:
			return ( Cmd_Write( unit, opener, sana ) );

		case CMD_FLUSH:
			Cmd_Flush( unit, IOERR_ABORTED, 0 );
			break;

		case S2_DEVICEQUERY:
			Cmd_DeviceQuery( sana );
			break;

		case S2_GETSTATIONADDRESS:
			Cmd_GetStationAddress( unit, sana );
			break;

		case S2_CONFIGINTERFACE:
			Cmd_ConfigInterface( unit, sana );
			break;

		case S2_ADDMULTICASTADDRESS:
		case S2_DELMULTICASTADDRESS:
		case S2_ADDMULTICASTADDRESSES:
		case S2_DELMULTICASTADDRESSES:
			Cmd_MulticastAddress( opener, sana );
			break;

		case S2_TRACKTYPE:
		case S2_UNTRACKTYPE:
		case S2_GETTYPESTATS:
			Cmd_TrackType( opener, sana );
			break;

		case S2_GETSPECIALSTATS:
			Sana2_GetSpecialStats( sana, NULL );
			break;

		case S2_GETGLOBALSTATS:
			Sana2_GetPacketStats( sana, unit->unit_PacketsRx, unit->unit_PacketsTx, unit->unit_BadPackets,
				unit->unit_UnknownPackets );
			break;

		case S2_GETEXTENDEDGLOBALSTATS:
			Sana2_GetExtendedPacketStats( sana, unit->unit_PacketsRx, unit->unit_PacketsTx, unit->unit_BadPackets,
				unit->unit_UnknownPackets, TRUE );
			break;

		case S2_ONEVENT:
		case S2_SAMPLE_THROUGHPUT:
			if ( unit->unit_Detached )
			{
				ReportUnitOffline( unit, sana );

				break;
			}

			if ( sana->ios2_Req.io_Command == S2_ONEVENT )
			{
				if ( Sana2_QueueEvent( & unit->unit_Pending, sana,
				unit->unit_Online ? S2EVENT_ONLINE : S2EVENT_OFFLINE, SANA2_COMMON_EVENTS ) )
			{
				sana->ios2_Req.io_Flags &= ~IOF_QUICK;

				return ( TRUE );
			}

			break;
			}

			if ( Sana2_QueueThroughput( & unit->unit_Pending, sana, NULL, TRUE ) )
			{
				sana->ios2_Req.io_Flags &= ~IOF_QUICK;

				return ( TRUE );
			}

			break;

		case S2_SANA2HOOK:
			Sana2_SetCopyHook( sana, & opener->bm_CopyHook, FALSE );
			break;

		case S2_GETCRYPTTYPES:
			Sana2_GetCryptTypes( sana, NULL, 0 );
			break;

		case S2_OFFLINE:
			if ( unit->unit_Online )
			{
				unit->unit_Online = FALSE;

				Cmd_Flush( unit, S2ERR_OUTOFSERVICE, S2WERR_UNIT_OFFLINE );

				Sana2_SignalEvent( & unit->unit_Pending, S2EVENT_OFFLINE, TRUE );
			}

			break;

		case S2_ONLINE:
			if ( ! unit->unit_Online )
			{
				ReportUnitOffline( unit, sana );
			}

			break;

		case S2_GETSIGNALQUALITY:
		case S2_GETNETWORKS:
		case S2_SETOPTIONS:
		case S2_SETKEY:
		case S2_GETNETWORKINFO:
		case S2_READMGMT:
		case S2_WRITEMGMT:
		case S2_GETRADIOBANDS:
		case USB2WIRELESS_CMD_PROMPT:
			ReportUnitOffline( unit, sana );
			break;

		case S2_GETPEERADDRESS:
		case S2_GETDNSADDRESS:
		case S2_CONNECT:
		case S2_DISCONNECT:
			sana->ios2_Req.io_Error = S2ERR_NOT_SUPPORTED;
			break;

		default:
			sana->ios2_Req.io_Error = IOERR_NOCMD;
			break;
	}

	return ( FALSE );
}

// --
