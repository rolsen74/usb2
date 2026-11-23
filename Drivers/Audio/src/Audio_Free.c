/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Audio_Free( struct AudioUnit *unit )
{
struct IOStdReq *ioreq;
struct AudioBase *base;

	if ( ! unit )
	{
		return;
	}

	base = unit->Base;

	ObtainSemaphore( & base->Semaphore );
	unit->Detached = TRUE;
	ReleaseSemaphore( & base->Semaphore );

	Audio_Read_Stop( unit );

	Audio_Write_Stop( unit, IOERR_ABORTED );

	Audio_Read_Reply( unit, IOERR_ABORTED );

	if ( ( unit->ActiveAlt ) && ( unit->Register ) )
	{
		if ( unit->Stream )
		{
			USB2_EPRes_Release( unit->Stream );

			unit->Stream = NULL;
		}

		USB2_Ifc_SetAlt( unit->Register, unit->StartMsg->Interface, 0 );

		unit->ActiveAlt = FALSE;
	}

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
		USB2_Reg_Unregister( unit->Register );
		unit->Register = NULL;
	}

	if ( unit->BeginPort )
	{
		FreeSysObject( ASOT_PORT, unit->BeginPort );
	}

	ObtainSemaphore( & base->Semaphore );
	unit->Released = TRUE;

	if ( unit->UnitNr < AUDIO_MAX_UNITS )
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
