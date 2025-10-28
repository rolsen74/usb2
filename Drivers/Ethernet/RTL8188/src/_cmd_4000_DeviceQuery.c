
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#include "All.h"

// --

/* Supported commands */
static const U16 _DevNSDCmds[] =
{
// Common device commands
/* 0000 */	//	CMD_INVALID,		// Not a real command
/* 0001 */	//	CMD_RESET,
/* 0002 */	//	CMD_READ,
/* 0003 */	CMD_WRITE,
/* 0004 */	//	CMD_UPDATE,
/* 0005 */	//	CMD_CLEAR,
/* 0006 */	//	CMD_STOP,
/* 0007 */	//	CMD_START,
/* 0008 */	//	CMD_FLUSH,
/* 0009	*/	//	CMD_NONSTD,			// Normal first Device Specific Command

// Parallel.dev commands
/* 0009 */	//	PDCMD_QUERY,			// CMD_NONSTD+0
/* 000A */	//	PDCMD_SETPARAMS,

/* 4000 */	NSCMD_DEVICEQUERY,
	0
};

void _cmd_4000_NSCmd_DeviceQuery( struct IOStdReq *ioreq )
{
struct NSDeviceQueryResult *nsd;

	USBDEBUG( "RTL8188 : NSCmd_DeviceQuery" );

	nsd = ioreq->io_Data;

	if (( nsd ) && ( ioreq->io_Length >= sizeof( struct NSDeviceQueryResult )))
	{
		nsd->DevQueryFormat		= 0;
		nsd->SizeAvailable		= sizeof( struct NSDeviceQueryResult );
		nsd->DeviceType			= NSDEVTYPE_PARALLEL;
		nsd->DeviceSubType		= 0;
		nsd->SupportedCommands	= (PTR) _DevNSDCmds;

		ioreq->io_Actual		= sizeof( struct NSDeviceQueryResult );
		ioreq->io_Error			= 0;
	}
	else
	{
		USBDEBUG( "RTL8188 : Supplied Query buffer is to small" );
		ioreq->io_Error			= IOERR_BADLENGTH;
	}

	ReplyMsg( (PTR) ioreq );
}

// --
