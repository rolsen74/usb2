
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Commands.h"
#include "EthClass.h"

// --

extern const UWORD EthClass_Commands[];

// --

static LONG
Open_Adapter( APTR adapter, struct IOSana2Req *request, ULONG flags )
{
	struct EthernetUnit *unit;
	struct BufferManagement *opener;

	LONG error;

	unit = adapter;
	error = IOERR_OPENFAIL;

	if ( flags & SANA2OPF_PROM )
	{
		return ( error );
	}

	ObtainSemaphore( & unit->unit_CommandLock );

	if ( ( unit->unit_Detached ) || ( unit->unit_Exclusive ) ||
		( ( unit->unit_Unit.unit_OpenCnt ) && ( flags & SANA2OPF_MINE ) ) )
	{
		goto bailout;
	}

	opener = AllocVecTags( sizeof( *opener ), AVT_Clear, 0, TAG_END );

	if ( ! opener )
	{
		goto bailout;
	}

	NewList( & opener->bm_Multicast );

	NewList( & opener->bm_TrackedTypes );

	if ( request->ios2_BufferManagement )
	{
		opener->bm_CopyToBuff = (PTR) GetTagData( S2_CopyToBuff, 0, request->ios2_BufferManagement );

		opener->bm_CopyFromBuff = (PTR) GetTagData( S2_CopyFromBuff, 0, request->ios2_BufferManagement );
	}

	AddTail( & unit->unit_Openers, & opener->bm_Node );

	request->ios2_BufferManagement = opener;
	unit->unit_Exclusive = ( flags & SANA2OPF_MINE ) != 0;
	unit->unit_Unit.unit_OpenCnt++;
	error = 0;

bailout:

	ReleaseSemaphore( & unit->unit_CommandLock );

	return ( error );
}

// --

static void
Close_Adapter( APTR adapter, struct IOSana2Req *request )
{
	struct EthernetUnit *unit;
	struct BufferManagement *opener;

	unit = adapter;

	ObtainSemaphore( & unit->unit_CommandLock );

	opener = FindOpener( unit, request->ios2_BufferManagement );

	if ( opener )
	{
		Sana2_AbortPending( & unit->unit_Pending, NULL, opener, IOERR_ABORTED, 0 );

		Remove( & opener->bm_Node );

		FreeOpener( opener );

		request->ios2_BufferManagement = NULL;
	}

	if ( ! --unit->unit_Unit.unit_OpenCnt )
	{
		unit->unit_Exclusive = FALSE;
	}

	ReleaseSemaphore( & unit->unit_CommandLock );
}

// --

static void
Begin_Adapter( APTR adapter, struct IOSana2Req *request )
{
	struct EthernetUnit *unit;

	S32 pending;

	unit = adapter;
	pending = FALSE;
	request->ios2_Req.io_Error = 0;

	ObtainSemaphore( & unit->unit_CommandLock );

	if ( unit->unit_Detached )
	{
		ReportUnitOffline( unit, request );
	}
	else
	{
		pending = Sana2BeginIO( unit, request );

		if ( pending )
		{
			request->ios2_Req.io_Message.mn_Node.ln_Type = NT_MESSAGE;
		}
	}

	ReleaseSemaphore( & unit->unit_CommandLock );

	if ( ( ! pending ) && ( ! ( request->ios2_Req.io_Flags & IOF_QUICK ) ) )
	{
		ReplyMsg( (PTR) request );
	}
}

// --

static void
Abort_Adapter( APTR adapter, struct IOSana2Req *request )
{
	struct EthernetUnit *unit;

	unit = adapter;

	ObtainSemaphore( & unit->unit_CommandLock );

	Sana2_AbortPending( & unit->unit_Pending, & request->ios2_Req, NULL, IOERR_ABORTED, 0 );

	ReleaseSemaphore( & unit->unit_CommandLock );
}

// --

static void
Destroy_Adapter( APTR adapter )
{
	FreeVec( adapter );
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
