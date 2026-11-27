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

	// Once STOP can report a detach, callers may free their USB buffers.
	Video_Read_Stop( unit );

	ObtainSemaphore( & base->Semaphore );

	unit->Detached = TRUE;

	if ( ( unit->UnitNr < VIDEO_MAX_UNITS ) && ( base->Slots[unit->UnitNr].Unit == unit ) )
	{
		base->Slots[unit->UnitNr].Unit = NULL;
	}

	ReleaseSemaphore( & base->Semaphore );

	Video_Read_Reply( unit, USB2Err_Device_Detached );

	if ( unit->BeginPort )
	{
		while(( ioreq = (PTR) GetMsg( unit->BeginPort ) ))
		{
			ioreq->io_Error = USB2Err_Device_Detached;
			ReplyMsg( (PTR) ioreq );
		}
	}

	if ( unit->Register )
	{
		Video_Stream_Close( unit );

		USB2_Reg_Unregister( unit->Register );
		unit->Register = NULL;
	}

	if ( unit->TimerOpen )
	{
		CloseDevice( & unit->Timer->Request );
	}

	if ( unit->Timer )
	{
		FreeSysObject( ASOT_IOREQUEST, unit->Timer );
	}

	if ( unit->TimerPort )
	{
		FreeSysObject( ASOT_PORT, unit->TimerPort );
	}

	if ( unit->StreamPort )
	{
		FreeSysObject( ASOT_PORT, unit->StreamPort );
	}

	if ( unit->BeginPort )
	{
		FreeSysObject( ASOT_PORT, unit->BeginPort );
	}

	FreeVec( unit );
}

// --
