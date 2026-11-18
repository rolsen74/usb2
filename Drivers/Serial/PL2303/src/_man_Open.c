/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE S32 Serial_Manager_Open( struct DeviceManagerInterface *Self, struct IORequest *request, U32 unitnr, U32 flags UNUSED )
{
struct SerialBase *base;
struct SerialUnit *unit;
struct IOExtSer *ioreq;

	base = (PTR) Self->Data.LibBase;
	ioreq = (PTR) request;
	unit = NULL;

	ObtainSemaphore( & base->Semaphore );

	if ( unitnr < SERIAL_MAX_UNITS )
	{
		unit = base->Units[unitnr];
	}

	if ( ( ! unit ) || ( unit->Detached ) || ( request->io_Message.mn_Length < sizeof( struct IOExtSer ) ) )
	{
		request->io_Error = IOERR_OPENFAIL;
		request->io_Device = NULL;
		goto bailout;
	}

	if ( unit->OpenCount )
	{
		if ( ( ! unit->Shared ) || ( ! ( ioreq->io_SerFlags & SERF_SHARED ) ) )
		{
			request->io_Error = IOERR_OPENFAIL;
			request->io_Device = NULL;
			goto bailout;
		}
	}
	else
	{
		unit->Shared = ( ioreq->io_SerFlags & SERF_SHARED ) ? TRUE : FALSE;
	}

	base->Library.lib_Flags &= ~LIBF_DELEXP;
	base->Library.lib_OpenCnt++;
	unit->OpenCount++;

	request->io_Message.mn_Node.ln_Type = NT_REPLYMSG;
	request->io_Unit = (PTR) unit;
	request->io_Error = 0;
	ioreq->io_CtlChar = SER_DEFAULT_CTLCHAR;
	ioreq->io_RBufLen = SERIAL_RX_SIZE;
	ioreq->io_ExtFlags = 0;
	ioreq->io_Baud = 9600;
	ioreq->io_BrkTime = 250000;
	ioreq->io_ReadLen = 8;
	ioreq->io_WriteLen = 8;
	ioreq->io_StopBits = 1;
	ioreq->io_TermArray.TermArray0 = 0;
	ioreq->io_TermArray.TermArray1 = 0;

bailout:
	ReleaseSemaphore( & base->Semaphore );

	return ( request->io_Error );
}

// --
