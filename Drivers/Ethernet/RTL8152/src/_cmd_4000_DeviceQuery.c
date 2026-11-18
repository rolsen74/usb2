
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

void
NSCmd_DeviceQuery( struct devUnit *unit UNUSED, struct IOStdReq *ioreq )
{
struct NSDeviceQueryResult *nsd;

	static uint16 _DevNSDCmds[] =
	{
		CMD_READ,
		CMD_WRITE,
		CMD_FLUSH,
		NSCMD_DEVICEQUERY,
		S2_SANA2HOOK,
		S2_DEVICEQUERY,
		S2_GETSTATIONADDRESS,
		S2_CONFIGINTERFACE,
	    S2_ADDMULTICASTADDRESS,
	    S2_DELMULTICASTADDRESS,
		S2_MULTICAST,
		S2_BROADCAST,
		S2_GETSPECIALSTATS,
		S2_GETGLOBALSTATS,
		S2_ONEVENT,
		S2_READORPHAN,
	    S2_ONLINE,
	    S2_OFFLINE,
		S2_GETEXTENDEDGLOBALSTATS,
		S2_SAMPLE_THROUGHPUT,
	    0
	};

	nsd = ioreq->io_Data;

	if (( nsd != NULL ) && ( ioreq->io_Length >= sizeof( struct NSDeviceQueryResult )))
    {
    	// (the size check is only valid because the current NSDDeviceQueryResult
    	//  structure is defined as the minimum version of that struct!)
		nsd->DevQueryFormat		= 0;
		nsd->SizeAvailable		= sizeof( struct NSDeviceQueryResult );
		nsd->DeviceType			= NSDEVTYPE_SANA2;
		nsd->DeviceSubType		= 0;
		nsd->SupportedCommands	= (APTR)_DevNSDCmds;

        ioreq->io_Actual		= sizeof( struct NSDeviceQueryResult );
        ioreq->io_Error			= 0;
    }
    else
    {
		SHOWMSG( "Supplied query buffer is too small" );

		ioreq->io_Actual = 0;
        ioreq->io_Error			= IOERR_BADLENGTH;
    }

	// NSD query performed
	IExec->ReplyMsg( (APTR) ioreq );

}

// --
