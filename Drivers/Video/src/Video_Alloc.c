/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

static S32
Video_Open_Default( struct VideoUnit *unit, const struct VideoMode *preferred )
{
	struct TimeVal time;
	struct VideoMode mode;

	U32 index;
	U32 attempts;
	S32 opened;
	S8 error;

	unit->ProbeClock = (PTR) GetInterface( (PTR) unit->Timer->Request.io_Device, "main", 1, NULL );

	if ( ! unit->ProbeClock )
	{
		return ( FALSE );
	}

	unit->ProbeClock->GetUpTime( & time );

	unit->ProbeDeadline = (U64) time.Seconds * 1000000 + time.Microseconds + 5000000;
	mode = *preferred;
	attempts = 0;
	index = 0;
	opened = FALSE;

	for ( ;; )
	{
		opened = Video_Stream_Open( unit, & mode );

		if ( opened )
		{
			unit->DefaultMode = mode;
			unit->DefaultMode.Format = unit->Format;

			break;
		}

		error = unit->Control->IORequests[0]->io_Error;

		if ( ( unit->OV534 ) || ( error == USB2Err_Device_Detached ) || ( error == USB2Err_Host_Timeout ) ||
			( error == USB2Err_Host_HostError ) || ( ++attempts == 4 ) )
		{
			break;
		}

		do
		{
			if ( ! Video_Parse( unit, ++index, & mode ) )
			{
				goto finished;
			}
		}
		while ( ( mode.FormatIndex == preferred->FormatIndex ) && ( mode.FrameIndex == preferred->FrameIndex ) &&
			( mode.Format.FrameInterval == preferred->Format.FrameInterval ) );

		DebugPrintF( "USB Video: %04lx:%04lx trying format %lu frame %lu interval %lu\n",
			mode.Format.VendorID, mode.Format.DeviceID, (U32) mode.FormatIndex, (U32) mode.FrameIndex,
			mode.Format.FrameInterval );
	}

finished:
	DropInterface( (PTR) unit->ProbeClock );

	unit->ProbeClock = NULL;
	unit->ProbeDeadline = 0;
	unit->Control->IORequests[0]->io_TimeOut = 0;

	return ( opened );
}

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
	unit->OV534 = ( msg->InterfaceDescriptor->InterfaceClass == USBCLASS_Vendor_Specific );

	if ( ( ! Video_Find_Endpoint( unit ) ) || ( ! Video_Parse( unit, 0, & mode ) ) )
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

	unit->TimerPort = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! unit->TimerPort )
	{
		goto bailout;
	}

	unit->Timer = AllocSysObjectTags( ASOT_IOREQUEST,
		ASOIOR_Size, sizeof( *unit->Timer ), ASOIOR_ReplyPort, unit->TimerPort, TAG_END );

	if ( ! unit->Timer )
	{
		goto bailout;
	}

	if ( OpenDevice( TIMERNAME, UNIT_MICROHZ, & unit->Timer->Request, 0 ) )
	{
		goto bailout;
	}

	unit->TimerOpen = TRUE;
	unit->Register = USB2_Reg_RegisterTags(
		USB2Tag_Reg_DriverMessage, msg,
		USB2Tag_Reg_Title, unit->OV534 ? "PlayStation Eye Video" : "USB Video",
		TAG_END
	);

	if ( ! unit->Register )
	{
		goto bailout;
	}

	unit->Control = unit->Register->Res_Control;

	if ( ! Video_Open_Default( unit, & mode ) )
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
