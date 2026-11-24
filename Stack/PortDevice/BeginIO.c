
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "PortDevice.h"

// --

SEC_CODE void
PortDevice_BeginIO( struct DeviceManagerInterface *Self, struct IOStdReq *request )
{
	struct USB2PortDevice *base;
	struct USB2PortUnit *unit;
	struct ExecIFace *IExec;

	base = (PTR) Self->Data.LibBase;
	IExec = base->IExec;
	unit = (PTR) request->io_Unit;
	request->io_Error = 0;

	if ( request->io_Message.mn_Length >= sizeof( *request ) )
	{
		request->io_Actual = 0;
	}

	if ( request->io_Command != NSCMD_DEVICEQUERY )
	{
		if ( ( request->io_Message.mn_Length < sizeof( *request ) ) ||
			( ( base->Type == PORT_SERIAL ) && ( request->io_Message.mn_Length < sizeof( struct IOExtSer ) ) ) ||
			( ( ( base->Type == PORT_PARALLEL ) ||
			    ( ( base->Type == PORT_PRINTER ) && ( request->io_Command == PDCMD_QUERY ) ) ) &&
			  ( request->io_Message.mn_Length < sizeof( struct IOExtPar ) ) ) )
		{
			request->io_Error = IOERR_BADLENGTH;
			goto reply;
		}
	}

	IExec->ObtainSemaphore( & base->Lock );

	if ( ( unit ) && ( unit->BeginPort ) && ( ( base->Type != PORT_PRINTER ) || ( ( ! unit->Closing ) && ( ! unit->Quiescing ) ) ) &&
		( ( request->io_Command != NSCMD_DEVICEQUERY ) ||
		  ( ( base->Type != PORT_PRINTER ) && ( request->io_Message.mn_Length >= sizeof( *request ) ) ) ) )
	{
		request->io_Flags &= ~( IOF_QUICK |
			( base->Type == PORT_SERIAL ? ( IOSERF_ABORT | IOSERF_QUEUED | IOSERF_ACTIVE ) :
			( IOPARF_ABORT | IOPARF_QUEUED | IOPARF_ACTIVE ) ) );
		request->io_Message.mn_Node.ln_Type = NT_MESSAGE;

		IExec->PutMsg( unit->BeginPort, (PTR) request );

		IExec->ReleaseSemaphore( & base->Lock );

		return;
	}

	IExec->ReleaseSemaphore( & base->Lock );

	if ( request->io_Command == NSCMD_DEVICEQUERY )
	{
		PortDevice_Query( base, request );
	}
	else if ( base->Type == PORT_PRINTER )
	{
		request->io_Error = ParErr_NotOpen;

		if ( request->io_Message.mn_Length >= sizeof( struct IOExtPar ) )
		{
			((struct IOExtPar *) request)->io_Status = 0;
		}
	}
	else
	{
		request->io_Error = IOERR_UNITBUSY;
	}

reply:

	if ( ! ( request->io_Flags & IOF_QUICK ) )
	{
		IExec->ReplyMsg( (PTR) request );
	}
}

// --
