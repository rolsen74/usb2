
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "PortDevice.h"

// --

SEC_CODE static U8
SharedOpen( struct USB2PortDevice *base, struct IORequest *request )
{
	if ( base->Serial )
	{
		return ( ( ((struct IOExtSer *) request)->io_SerFlags & SERF_SHARED ) != 0 );
	}

	return ( ( ((struct IOExtPar *) request)->io_ParFlags & PARF_SHARED ) != 0 );
}

// --

SEC_CODE static void
SerialDefaults( struct IOExtSer *request )
{
	request->io_CtlChar = SER_DEFAULT_CTLCHAR;
	request->io_RBufLen = 4096;
	request->io_ExtFlags = 0;
	request->io_Baud = 9600;
	request->io_BrkTime = 250000;
	request->io_ReadLen = 8;
	request->io_WriteLen = 8;
	request->io_StopBits = 1;
	request->io_TermArray.TermArray0 = 0;
	request->io_TermArray.TermArray1 = 0;
}

// --

SEC_CODE S32
PortDevice_Open( struct DeviceManagerInterface *Self, struct IORequest *request, U32 number, U32 flags UNUSED )
{
	struct USB2PortDevice *base;
	struct USB2PortUnit *unit;
	struct ExecIFace *IExec;

	U8 shared;
	U32 size;

	base = (PTR) Self->Data.LibBase;
	IExec = base->IExec;
	unit = NULL;
	request->io_Unit = NULL;
	request->io_Error = IOERR_OPENFAIL;

	IExec->ObtainSemaphore( & base->Lock );

	if ( number != USB2_PORT_BACKEND_UNIT )
	{
		size = base->Serial ? sizeof( struct IOExtSer ) : sizeof( struct IOExtPar );

		if ( request->io_Message.mn_Length < size )
		{
			goto bailout;
		}

		unit = base->Units;

		while( ( unit ) && ( unit->Number < number ) )
		{
			unit = unit->Next;
		}

		if ( ( ! unit ) || ( unit->Number != number ) )
		{
			goto bailout;
		}

		shared = SharedOpen( base, request );

		if ( ( unit->Opens ) && ( ( ! unit->Shared ) || ( ! shared ) ) )
		{
			goto bailout;
		}

		unit->Shared = shared;
		unit->Opens++;

		if ( base->Serial )
		{
			SerialDefaults( (PTR) request );
		}
	}

	base->Library.lib_OpenCnt++;
	request->io_Device = (PTR) base;
	request->io_Unit = (PTR) unit;
	request->io_Message.mn_Node.ln_Type = NT_REPLYMSG;
	request->io_Error = 0;

bailout:
	if ( request->io_Error )
	{
		request->io_Device = NULL;
	}

	IExec->ReleaseSemaphore( & base->Lock );

	return ( request->io_Error );
}

// --
