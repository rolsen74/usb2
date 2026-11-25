
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "EthClass.h"

// --

extern const UWORD EthClass_Commands[];

// --

static LONG
Open_Adapter( APTR adapter, struct IOSana2Req *request, ULONG flags )
{
	struct devUnit *unit;
	struct BufferManagement *opener;
	struct TagItem *tags;

	unit = adapter;
	tags = request->ios2_BufferManagement;

	IExec->ObtainSemaphore( & unit->unit_DispatchLock );

	opener = NULL;

	if ( ( unit->unit_AcceptIO ) && ( ! unit->unit_Exclusive ) && ( ! ( flags & SANA2OPF_PROM ) ) &&
		( ( ! unit->unit_Base.unit_OpenCnt ) || ( ! ( flags & SANA2OPF_MINE ) ) ) )
	{
		opener = IExec->AllocVecTags( sizeof( *opener ), AVT_Clear, 0, TAG_END );

		if ( opener )
		{
			unit->unit_Base.unit_OpenCnt++;
			unit->unit_Exclusive = ( flags & SANA2OPF_MINE ) != 0;
		}
	}

	IExec->ReleaseSemaphore( & unit->unit_DispatchLock );

	if ( ! opener )
	{
		return ( IOERR_OPENFAIL );
	}

	if ( tags )
	{
		opener->bm_CopyToBuff = (PTR) IUtility->GetTagData( S2_CopyToBuff, 0, tags );

		opener->bm_CopyFromBuff = (PTR) IUtility->GetTagData( S2_CopyFromBuff, 0, tags );

		opener->bm_PacketHook = (PTR) IUtility->GetTagData( S2_PacketFilter, 0, tags );
	}

	request->ios2_BufferManagement = opener;

	return ( 0 );
}

// --

static void
Close_Adapter( APTR adapter, struct IOSana2Req *request )
{
	struct devUnit *unit;

	unit = adapter;

	IExec->ObtainSemaphore( & unit->unit_DispatchLock );

	if ( ! --unit->unit_Base.unit_OpenCnt )
	{
		unit->unit_Exclusive = FALSE;
	}

	IExec->ReleaseSemaphore( & unit->unit_DispatchLock );

	IExec->FreeVec( request->ios2_BufferManagement );

	request->ios2_BufferManagement = NULL;
}

// --

static void
Begin_Adapter( APTR adapter, struct IOSana2Req *request )
{
	struct devUnit *unit;
	struct SanaRequest *submission;

	unit = adapter;
	request->ios2_Req.io_Flags &= ~IOF_QUICK;
	request->ios2_Req.io_Message.mn_Node.ln_Type = NT_MESSAGE;

	IExec->ObtainSemaphore( & unit->unit_DispatchLock );

	if ( unit->unit_AcceptIO )
	{
		if ( FindSanaRequest( unit, request ) )
		{
			IExec->ReleaseSemaphore( & unit->unit_DispatchLock );

			return;
		}

		submission = IExec->AllocVecTags( sizeof( *submission ), AVT_Clear, 0, TAG_END );

		if ( submission )
		{
			submission->Request = request;

			IExec->AddTail( & unit->unit_Requests, & submission->Node );

			IExec->PutMsg( unit->unit_BeginIOMsgPort, (PTR) request );

			IExec->ReleaseSemaphore( & unit->unit_DispatchLock );

			return;
		}

		request->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
		request->ios2_WireError = S2WERR_GENERIC_ERROR;
	}
	else
	{
		request->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
		request->ios2_WireError = S2WERR_UNIT_DISCONNECTED;
	}

	IExec->ReleaseSemaphore( & unit->unit_DispatchLock );

	IExec->ReplyMsg( (PTR) request );
}

// --

static void
Abort_Adapter( APTR adapter, struct IOSana2Req *request )
{
	struct devUnit *unit;
	struct SanaRequest *submission;

	unit = adapter;

	IExec->ObtainSemaphore( & unit->unit_DispatchLock );

	submission = unit->unit_AcceptIO ? FindSanaRequest( unit, request ) : NULL;

	if ( submission )
	{
		submission->Cancelled = TRUE;

		IExec->Signal( unit->unit_BeginIOMsgPort->mp_SigTask, unit->unit_BeginIOMsgPortBit );
	}

	IExec->ReleaseSemaphore( & unit->unit_DispatchLock );
}

// --

static void
Destroy_Adapter( APTR adapter )
{
	IExec->FreeVec( adapter );
}

// --

const struct USB2EthOps EthClass_Ops =
{
	.Commands = EthClass_Commands,
	.Open = Open_Adapter,
	.Close = Close_Adapter,
	.Begin = Begin_Adapter,
	.Abort = Abort_Adapter,
	.Destroy = Destroy_Adapter
};

// --
