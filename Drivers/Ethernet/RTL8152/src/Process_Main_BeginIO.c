
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

static S32
StartSanaRequest( struct devUnit *unit, struct IOSana2Req *request )
{
	struct SanaRequest *submission;

	S32 start;

	IExec->ObtainSemaphore( & unit->unit_DispatchLock );

	submission = FindSanaRequest( unit, request );

	start = FALSE;

	if ( submission )
	{
		submission->Started = TRUE;
		start = ! submission->Cancelled;
	}

	IExec->ReleaseSemaphore( & unit->unit_DispatchLock );

	return ( start );
}

// --

void
Handle_BeginIOMessages( struct devUnit *unit )
{
struct IOSana2Req *ioreq;

	while(( ioreq = (APTR) IExec->GetMsg( unit->unit_BeginIOMsgPort )))
	{
		if ( ! StartSanaRequest( unit, ioreq ) )
		{
			ReplySanaRequest( unit, ioreq );

			continue;
		}

		ioreq->ios2_Req.io_Error = 0;

		switch( ioreq->ios2_Req.io_Command )
		{
		    case /* 0000 */ CMD_INVALID:
		    {
			    SHOWMSG( "BeginIO: io_Command = CMD_INVALID" );

				Cmd_Invalid( unit, ioreq );
			    break;
		    }

		    case /* 0002 */ CMD_READ:
		    {
			    SHOWMSG( "BeginIO: io_Command = CMD_READ" );

				Cmd_Read( unit, ioreq );
			    break;
		    }

		    case /* 0003 */ CMD_WRITE:
		    {
			    SHOWMSG( "BeginIO: io_Command = CMD_WRITE" );

				Cmd_Write( unit, ioreq );
			    break;
		    }

		    case /* 0008 */ CMD_FLUSH:
		    {
			    SHOWMSG( "BeginIO: io_Command = CMD_FLUSH" );

                Cmd_Flush( unit, ioreq );
			    break;
		    }

		    case /* 0009 */ S2_DEVICEQUERY:
		    {
				SHOWMSG( "BeginIO: io_Command = S2_DEVICEQUERY" );

                s2_DeviceQuery( unit, ioreq );
			    break;
		    }

		    case /* 000A */ S2_GETSTATIONADDRESS:
		    {
				SHOWMSG( "BeginIO: io_Command = S2_GETSTATIONADDRESS" );

                s2_GetStationAddress( unit, ioreq );
			    break;
		    }

		    case /* 000B */ S2_CONFIGINTERFACE:
		    {
				SHOWMSG( "BeginIO: io_Command = S2_CONFIGINTERFACE" );

                s2_ConfigInterface( unit, ioreq );
			    break;
		    }

			case /* 000E */ S2_ADDMULTICASTADDRESS:
			{
				s2_AddMultiCastAddress( unit, ioreq );

				break;
			}

			case /* 000F */ S2_DELMULTICASTADDRESS:
			{
				s2_DelMultiCastAddress( unit, ioreq );

				break;
			}

			case /* 0010 */ S2_MULTICAST:
		    {
			    SHOWMSG( "BeginIO: io_Command = S2_MULTICAST" );

                s2_MultiCast( unit, ioreq );
			    break;
		    }

			case /* 0011 */ S2_BROADCAST:
		    {
			    SHOWMSG( "BeginIO: io_Command = S2_BROADCAST" );

				s2_BroadCast( unit, ioreq );
			    break;
		    }

			case /* 0015 */ S2_GETSPECIALSTATS:
			{
				s2_GetSpecialStats( unit, ioreq );

				break;
			}

			case /* 0016 */ S2_GETGLOBALSTATS:
			{
				s2_GetGlobalStats( unit, ioreq );

				break;
			}

			case S2_ADDMULTICASTADDRESSES:
			case S2_DELMULTICASTADDRESSES:
				Sana2_MulticastRange( & unit->unit_MulticastRanges, ioreq );

				ReplySanaRequest( unit, (PTR) ioreq );

				break;

			case S2_TRACKTYPE:
			case S2_UNTRACKTYPE:
			case S2_GETTYPESTATS:
				Sana2_TrackType( & unit->unit_TrackedTypes, ioreq );

				ReplySanaRequest( unit, (PTR) ioreq );

				break;

			case /* 0017 */ S2_ONEVENT:
			{
				s2_OnEvent( unit, ioreq );

				break;
			}

			case /* 0018 */ S2_READORPHAN:
		    {
			    SHOWMSG( "BeginIO: io_Command = S2_READORPHAN" );

                s2_ReadOrphan( unit, ioreq );
			    break;
		    }

			case /* 0019 */ S2_ONLINE:
		    {
			    SHOWMSG( "BeginIO: io_Command = S2_ONLINE" );

				s2_Online( unit, ioreq );
			    break;
		    }

			case /* 001A */ S2_OFFLINE:
		    {
			    SHOWMSG( "BeginIO: io_Command = S2_OFFLINE" );

                s2_Offline( unit, ioreq );
			    break;
		    }

			case /* C004 */ S2_GETEXTENDEDGLOBALSTATS:
			{
				s2_GetExtendedGlobalStats( unit, ioreq );

				break;
			}

			case /* C007 */ S2_SAMPLE_THROUGHPUT:
			{
				s2_Sample_ThroughPut( unit, ioreq );

				break;
			}

		    case /* C008 */ S2_SANA2HOOK:
		    {
				SHOWMSG( "BeginIO: io_Command = S2_SANA2HOOK" );

                s2_Sana2Hook( unit, ioreq );
			    break;
		    }

    		default:
            {
				SHOWMSG( "Unknown Command (%04lx) -- This should really not chould happen", ioreq->ios2_Req.io_Command );
				ioreq->ios2_Req.io_Error = IOERR_NOCMD;
				ReplySanaRequest( unit, (PTR) ioreq );
				break;
			}
		}
	}

}

// --
