
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
Open_Adapter( APTR adapter, struct IOSana2Req *request, ULONG flags UNUSED )
{
	struct devUnit *unit;
	struct BufferManagement *opener;
	struct TagItem *tags;

	unit = adapter;
	tags = request->ios2_BufferManagement;

	IExec->ObtainSemaphore( & unit->unit_DispatchLock );

	opener = NULL;

	if ( unit->unit_AcceptIO )
	{
		opener = IExec->AllocVecTags( sizeof( *opener ), AVT_Clear, 0, TAG_END );
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
Close_Adapter( APTR adapter UNUSED, struct IOSana2Req *request )
{
	IExec->FreeVec( request->ios2_BufferManagement );

	request->ios2_BufferManagement = NULL;
}

// --

static void
Begin_Adapter( APTR adapter, struct IOSana2Req *request )
{
	struct devUnit *unit;

	unit = adapter;
	request->ios2_Req.io_Flags &= ~IOF_QUICK;
	request->ios2_Req.io_Message.mn_Node.ln_Type = NT_MESSAGE;

	IExec->ObtainSemaphore( & unit->unit_DispatchLock );

	if ( unit->unit_AcceptIO )
	{
		IExec->PutMsg( unit->unit_BeginIOMsgPort, (PTR) request );

		IExec->ReleaseSemaphore( & unit->unit_DispatchLock );

		return;
	}

	IExec->ReleaseSemaphore( & unit->unit_DispatchLock );

	request->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
	request->ios2_WireError = S2WERR_UNIT_DISCONNECTED;

	IExec->ReplyMsg( (PTR) request );
}

// --

static void
Abort_Adapter( APTR adapter, struct IOSana2Req *request )
{
	struct AbortIOMessage *msg;
	struct devUnit *unit;

	unit = adapter;

	msg = IExec->AllocVecTags( sizeof( *msg ), AVT_Clear, 0, TAG_END );

	if ( ! msg )
	{
		return;
	}

	IExec->ObtainSemaphore( & unit->unit_DispatchLock );

	if ( ( unit->unit_AcceptIO ) && ( ! IExec->CheckIO( (PTR) request ) ) )
	{
		msg->ioreq = request;

		IExec->PutMsg( unit->unit_AbortIOMsgPort, (PTR) msg );

		msg = NULL;
	}

	IExec->ReleaseSemaphore( & unit->unit_DispatchLock );

	IExec->FreeVec( msg );
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
