/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE struct VideoUnit *Video_Alloc( struct VideoContext *ctx, struct USB2_DriverMessage *msg )
{
struct VideoUnit *unit;
struct VideoBase *base;
struct VideoMode mode;

U32 index;

	base = ctx->Device;
	unit = AllocVecTags( sizeof( struct VideoUnit ), AVT_ClearWithValue, 0, TAG_END );

	if ( ! unit )
	{
		return ( NULL );
	}

	unit->Context = ctx;
	unit->Base = base;
	unit->StartMsg = msg;
	unit->UnitNr = VIDEO_MAX_UNITS;
	unit->InterfaceNr = msg->InterfaceDescriptor->InterfaceNumber;

	if ( ! Video_Parse( unit, 0, & mode ) )
	{
		goto bailout;
	}

	unit->BeginPort = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! unit->BeginPort )
	{
		goto bailout;
	}

	unit->StreamPort = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! unit->StreamPort )
	{
		goto bailout;
	}

	unit->Register = USB2_Reg_RegisterTags(
		USB2Tag_Reg_DriverMessage, msg,
		USB2Tag_Reg_Title, "USB Video",
		TAG_END
	);

	if ( ! unit->Register )
	{
		goto bailout;
	}

	unit->Control = unit->Register->Res_Control;

	if ( ! Video_Stream_Open( unit, & mode ) )
	{
		goto bailout;
	}

	ObtainSemaphore( & base->Semaphore );

	for( index = 0; index < VIDEO_MAX_UNITS; index++ )
	{
		if ( ! base->Units[index] )
		{
			base->Units[index] = unit;
			unit->UnitNr = index;
			break;
		}
	}

	ReleaseSemaphore( & base->Semaphore );

	if ( unit->UnitNr == VIDEO_MAX_UNITS )
	{
		goto bailout;
	}

	return ( unit );

bailout:
	Video_Free( unit );

	return ( NULL );
}

// --
