
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Sana2Commands.h"

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

	if ( request->ios2_BufferManagement )
	{
		opener->bm_CopyToBuff = (PTR) GetTagData( S2_CopyToBuff, 0, request->ios2_BufferManagement );

		opener->bm_CopyFromBuff = (PTR) GetTagData( S2_CopyFromBuff, 0, request->ios2_BufferManagement );
	}

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
	opener = request->ios2_BufferManagement;

	#ifdef ETH_CLASS_HAS_WORK_LOCK
	ObtainSemaphore( & unit->unit_WorkLock );
	#endif

	ObtainSemaphore( & unit->unit_CommandLock );

	if ( opener )
	{
		Sana2_AbortPending( & unit->unit_Pending, NULL, opener, IOERR_ABORTED, 0 );

		FreeVec( opener );

		request->ios2_BufferManagement = NULL;
	}

	if ( ! --unit->unit_Unit.unit_OpenCnt )
	{
		unit->unit_Exclusive = FALSE;
	}

	ReleaseSemaphore( & unit->unit_CommandLock );

	#ifdef ETH_CLASS_HAS_WORK_LOCK
	ReleaseSemaphore( & unit->unit_WorkLock );
	#endif
}

// --

static void
Begin_Adapter( APTR adapter, struct IOSana2Req *request )
{
	request->ios2_Req.io_Flags &= ~IOF_QUICK;
	request->ios2_Req.io_Message.mn_Node.ln_Type = NT_MESSAGE;

	Sana2BeginIO( adapter, request );
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
	Unit_Free( adapter );
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
