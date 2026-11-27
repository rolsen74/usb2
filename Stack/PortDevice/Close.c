
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
	struct USBBase *usbbase;
	struct USB2PortUnit *unit;
	struct ExecIFace *IExec;
	struct MsgPort *reply;
	struct MsgPort fallback;
	struct Message close;

	BOOL wait;
	U32 pending;

	base = (PTR) Self->Data.LibBase;
	IExec = base->IExec;
	usbbase = base->USB;
	unit = (PTR) request->io_Unit;
	reply = NULL;
	wait = FALSE;
	pending = 0;

	if ( ( unit ) && ( ( base->Type == PORT_PRINTER ) || ( unit->SerialSessions ) ) )
	{
		reply = IExec->AllocSysObjectTags( ASOT_PORT, TAG_END );

		if ( ! reply )
		{
			// The reply message is the completion condition, even on Ctrl-F.
			pending = IExec->SetSignal( 0, 0 ) & SIGBREAKF_CTRL_F;

			MEM_SET( &fallback, 0, sizeof( fallback ) );

			IExec->NewList( &fallback.mp_MsgList );

			fallback.mp_Node.ln_Type = NT_MSGPORT;
			fallback.mp_Flags = PA_SIGNAL;
			fallback.mp_SigBit = SIGBREAKB_CTRL_F;

			fallback.mp_SigTask = IExec->FindTask( NULL );

			reply = &fallback;
		}

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
			PortDevice_UnlinkUnit( base, unit );

			IExec->FreeVec( unit );
		}
		else if ( ( ( base->Type == PORT_PRINTER ) || ( unit->SerialSessions ) ) && ( ! unit->Opens ) )
		{
			unit->Closing = TRUE;

			unit->CloseReply = &close;
			wait = TRUE;

			IExec->Signal( unit->AbortPort->mp_SigTask, 1UL << unit->AbortPort->mp_SigBit );
		}
	}

	IExec->ReleaseSemaphore( & base->Lock );

	if ( wait )
	{
		IExec->WaitPort( reply );

		IExec->GetMsg( reply );
	}

	if ( reply == &fallback )
	{
		if ( wait )
		{
			IExec->SetSignal( pending, SIGBREAKF_CTRL_F );
		}
	}
	else if ( reply )
	{
		IExec->FreeSysObject( ASOT_PORT, reply );
	}

	return ( NULL );
}

// --
