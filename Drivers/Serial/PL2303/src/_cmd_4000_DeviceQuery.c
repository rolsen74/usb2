/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_RODATA static const U16 Serial_Commands[] =
{
	CMD_RESET,
	CMD_READ,
	CMD_WRITE,
	CMD_CLEAR,
	CMD_STOP,
	CMD_START,
	CMD_FLUSH,
	SDCMD_QUERY,
	SDCMD_BREAK,
	SDCMD_SETPARAMS,
	NSCMD_DEVICEQUERY,
	0
};

// --

SEC_CODE void Serial_Cmd_DeviceQuery( struct SerialContext *ctx, struct SerialUnit *unit UNUSED, struct IOStdReq *ioreq )
{
struct NSDeviceQueryResult *result;

	result = ioreq->io_Data;

	if ( ( ! result ) || ( ioreq->io_Length < sizeof( struct NSDeviceQueryResult ) ) )
	{
		ioreq->io_Error = IOERR_BADLENGTH;
		Serial_Reply( ctx, (PTR) ioreq );

		return;
	}

	result->DevQueryFormat = 0;
	result->SizeAvailable = sizeof( struct NSDeviceQueryResult );
	result->DeviceType = NSDEVTYPE_SERIAL;
	result->DeviceSubType = 0;
	result->SupportedCommands = (PTR) Serial_Commands;
	ioreq->io_Actual = sizeof( struct NSDeviceQueryResult );

	Serial_Reply( ctx, (PTR) ioreq );
}

// --
