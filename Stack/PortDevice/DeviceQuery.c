
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "PortDevice.h"

// --

SEC_RODATA static const U16 SerialCommands[] =
{
	NSCMD_DEVICEQUERY,
	USB2_CMD_ENUM_UNITS,
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
	0
};

SEC_RODATA static const U16 ParallelCommands[] =
{
	NSCMD_DEVICEQUERY,
	USB2_CMD_ENUM_UNITS,
	0
};

SEC_RODATA static const U16 PrinterCommands[] =
{
	NSCMD_DEVICEQUERY,
	USB2_CMD_ENUM_UNITS,
	CMD_WRITE,
	PDCMD_QUERY,
	0
};

// --

SEC_CODE void
PortDevice_Query( struct USB2PortDevice *base, struct IOStdReq *request )
{
	if ( base->Type == PORT_SERIAL )
	{
		USB2_DeviceQuery( request, NSDEVTYPE_SERIAL, SerialCommands );
	}
	else if ( base->Type == PORT_PRINTER )
	{
		USB2_DeviceQuery( request, NSDEVTYPE_PRINTER, PrinterCommands );
	}
	else
	{
		USB2_DeviceQuery( request, NSDEVTYPE_PARALLEL, ParallelCommands );
	}
}

// --
