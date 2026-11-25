
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "EthDevice.h"

// --

SEC_CODE void
EthDevice_BeginIO( struct DeviceManagerInterface *Self, struct IORequest *request )
{
	struct USB2EthDevice *base;
	struct USB2EthUnit *unit;
	struct ExecIFace *IExec;

	const U16 *commands;
	S32 detached;

	base = (PTR) Self->Data.LibBase;
	IExec = base->USB->usb_IExec;

	IExec->ObtainSemaphore( & base->RouteLock );

	unit = (PTR) request->io_Unit;
	commands = unit ? unit->Ops->Commands : NULL;
	detached = unit && unit->Detached;

	IExec->ReleaseSemaphore( & base->RouteLock );

	if ( request->io_Command == NSCMD_DEVICEQUERY )
	{
		EthDevice_Query( (PTR) request, commands );
	}
	else if ( request->io_Command == USB2_ETH_CMD_ENUM_ADAPTERS )
	{
		if ( request->io_Message.mn_Length < sizeof( struct IOStdReq ) )
		{
			request->io_Error = IOERR_BADLENGTH;
		}
		else
		{
			EthDevice_EnumAdapters( base, (PTR) request );
		}
	}
	else if ( request->io_Message.mn_Length < sizeof( struct IOSana2Req ) )
	{
		request->io_Error = EthDevice_KnowsCommand( commands, request->io_Command ) ? IOERR_BADLENGTH : IOERR_NOCMD;
	}
	else if ( ( unit ) && ( ! detached ) )
	{
		unit->Ops->Begin( unit->Adapter, (PTR) request );

		return;
	}
	else if ( request->io_Command == S2_DEVICEQUERY )
	{
		EthDevice_Sana2Query( base->USB, (PTR) request );
	}
	else if ( EthDevice_KnowsCommand( commands, request->io_Command ) )
	{
		((struct IOSana2Req *) request)->ios2_WireError = detached ? S2WERR_UNIT_DISCONNECTED : S2WERR_UNIT_OFFLINE;
		request->io_Error = S2ERR_OUTOFSERVICE;
	}
	else
	{
		request->io_Error = IOERR_NOCMD;
	}

	if ( ! ( request->io_Flags & IOF_QUICK ) )
	{
		IExec->ReplyMsg( (PTR) request );
	}
}

// --
