
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

	base = (PTR) Self->Data.LibBase;
	IExec = base->USB->usb_IExec;

	IExec->ObtainSemaphore( & base->RouteLock );

	unit = (PTR) request->io_Unit;

	if ( request->io_Command == NSCMD_DEVICEQUERY )
	{
		commands = unit ? unit->Ops->Commands : NULL;

		if ( ( ! unit ) && ( base->Units[0] ) )
		{
			commands = base->Units[0]->Ops->Commands;
		}

		IExec->ReleaseSemaphore( & base->RouteLock );

		EthDevice_Query( (PTR) request, commands );

		goto reply;
	}

	if ( ( unit ) && ( unit->Detached ) )
	{
		IExec->ReleaseSemaphore( & base->RouteLock );

		if ( request->io_Message.mn_Length >= sizeof( struct IOSana2Req ) )
		{
			((struct IOSana2Req *) request)->ios2_WireError = S2WERR_UNIT_DISCONNECTED;
		}

		request->io_Error = S2ERR_OUTOFSERVICE;

		goto reply;
	}

	IExec->ReleaseSemaphore( & base->RouteLock );

	if ( unit )
	{
		if ( request->io_Message.mn_Length >= sizeof( struct IOSana2Req ) )
		{
			unit->Ops->Begin( unit->Adapter, (PTR) request );

			return;
		}

		request->io_Error = IOERR_BADLENGTH;
	}
	else if ( request->io_Message.mn_Length >= sizeof( struct IOSana2Req ) )
	{
		struct IOSana2Req *sana;

		sana = (PTR) request;
		request->io_Error = S2ERR_OUTOFSERVICE;
		sana->ios2_WireError = S2WERR_UNIT_OFFLINE;
	}
	else
	{
		request->io_Error = IOERR_NOCMD;
	}

reply:
	if ( ! ( request->io_Flags & IOF_QUICK ) )
	{
		IExec->ReplyMsg( (PTR) request );
	}
}

// --
