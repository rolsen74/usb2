
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "PortDevice.h"

// --

SEC_CODE PTR
PortDevice_Close( struct DeviceManagerInterface *Self, struct IORequest *request )
{
	struct USB2PortDevice *base;
	struct USB2PortUnit *unit;
	struct ExecIFace *IExec;
	struct MsgPort *reply;
	struct Message close;

	BOOL wait;

	base = (PTR) Self->Data.LibBase;
	IExec = base->IExec;
	unit = (PTR) request->io_Unit;
	reply = NULL;
	wait = FALSE;

	if ( ( unit ) && ( base->Type == PORT_PRINTER ) )
	{
		reply = IExec->AllocSysObjectTags( ASOT_PORT, TAG_END );

		close.mn_Node.ln_Type = NT_MESSAGE;
		close.mn_Length = sizeof( close );
		close.mn_ReplyPort = reply;
	}

	IExec->ObtainSemaphore( & base->Lock );

	base->Library.lib_OpenCnt--;
	request->io_Unit = NULL;
	request->io_Device = NULL;

	if ( unit )
	{
		unit->Opens--;

		if ( ( ! unit->BeginPort ) && ( ! unit->Opens ) )
		{
			IExec->FreeVec( unit );
		}
		else if ( ( base->Type == PORT_PRINTER ) && ( ! unit->Opens ) )
		{
			unit->Closing = TRUE;

			if ( reply )
			{
				unit->CloseReply = & close;
				wait = TRUE;
			}

			IExec->Signal( unit->AbortPort->mp_SigTask, 1UL << unit->AbortPort->mp_SigBit );
		}
	}

	IExec->ReleaseSemaphore( & base->Lock );

	if ( wait )
	{
		IExec->WaitPort( reply );

		IExec->GetMsg( reply );
	}

	if ( reply )
	{
		IExec->FreeSysObject( ASOT_PORT, reply );
	}

	return ( NULL );
}

// --
