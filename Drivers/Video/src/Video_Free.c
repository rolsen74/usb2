/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Video_Free( struct VideoUnit *unit )
{
struct IOStdReq *ioreq;
struct VideoBase *base;

	if ( ! unit )
	{
		return;
	}

	base = unit->Base;

	ObtainSemaphore( & base->Semaphore );
	unit->Detached = TRUE;
	ReleaseSemaphore( & base->Semaphore );

	Video_Read_Stop( unit );

	Video_Read_Reply( unit, IOERR_ABORTED );

	if ( unit->BeginPort )
	{
		while(( ioreq = (PTR) GetMsg( unit->BeginPort ) ))
		{
			ioreq->io_Error = IOERR_ABORTED;
			ReplyMsg( (PTR) ioreq );
		}
	}

	if ( unit->Register )
	{
		Video_Stream_Close( unit );

		USB2_Reg_Unregister( unit->Register );
		unit->Register = NULL;
	}

	if ( unit->StreamPort )
	{
		FreeSysObject( ASOT_PORT, unit->StreamPort );
	}

	if ( unit->BeginPort )
	{
		FreeSysObject( ASOT_PORT, unit->BeginPort );
	}

	ObtainSemaphore( & base->Semaphore );
	unit->Released = TRUE;

	if ( unit->UnitNr < VIDEO_MAX_UNITS )
	{
		base->Units[unit->UnitNr] = NULL;
	}

	if ( ! unit->OpenCount )
	{
		FreeVec( unit );
	}

	ReleaseSemaphore( & base->Semaphore );
}

// --
