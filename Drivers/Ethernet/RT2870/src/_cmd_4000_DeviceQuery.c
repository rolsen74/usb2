
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

/* Supported commands */
static const U16 _DevNSDCmds[] =
{
	CMD_READ,
	CMD_WRITE,
	CMD_FLUSH,
	S2_DEVICEQUERY,
	S2_GETSTATIONADDRESS,
	S2_CONFIGINTERFACE,
	S2_ONEVENT,
	S2_BROADCAST,
	S2_MULTICAST,
	S2_SANA2HOOK,
	S2_ONLINE,
	S2_OFFLINE,
	NSCMD_DEVICEQUERY,
	S2_SETOPTIONS,
	S2_SETKEY,
	USB2WIRELESS_CMD_PROMPT,
	S2_READMGMT,
	S2_WRITEMGMT,
	0
};

void
_cmd_4000_NSCmd_DeviceQuery( struct IOStdReq *ioreq )
{
	struct NSDeviceQueryResult *nsd;

	USBDEBUG( "RT2870 : NSCmd_DeviceQuery" );

	nsd = ioreq->io_Data;

	if (( nsd ) && ( ioreq->io_Length >= sizeof( struct NSDeviceQueryResult )))
	{
		nsd->DevQueryFormat		= 0;
		nsd->SizeAvailable		= sizeof( struct NSDeviceQueryResult );
		nsd->DeviceType			= NSDEVTYPE_SANA2;
		nsd->DeviceSubType		= 0;
		nsd->SupportedCommands	= (PTR) _DevNSDCmds;

		ioreq->io_Actual		= sizeof( struct NSDeviceQueryResult );
		ioreq->io_Error			= 0;
	}
	else
	{
		USBDEBUG( "RT2870 : Supplied query buffer is too small" );

		ioreq->io_Actual		= 0;
		ioreq->io_Error			= IOERR_BADLENGTH;
	}

	ReplyMsg( (PTR) ioreq );
}

// --
