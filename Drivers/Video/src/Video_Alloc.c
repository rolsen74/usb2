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

static void
Video_GetIdentity( struct VideoUnit *unit, struct VideoIdentity *identity )
{
	struct USB2_DriverMessage *msg;

	U32 length;

	msg = unit->StartMsg;
	identity->Port = (struct USB2_PortPath) { 0 };
	identity->VendorID = msg->VendorID;
	identity->DeviceID = msg->DeviceID;
	identity->InterfaceNr = unit->InterfaceNr;
	identity->SerialLength = 0;

	if ( ! msg->Function )
	{
		return;
	}

	length = USB2_Attr_Get( msg->Function->FKT_ID, USB2Tag_Attr_SerialStr, identity->Serial, sizeof( identity->Serial ) );

	if ( ( length >= 4 ) && ( length <= sizeof( identity->Serial ) ) && ( ! ( length & 1 ) ) &&
		( identity->Serial[0] == length ) && ( identity->Serial[1] == DSCTYPE_String ) )
	{
		identity->SerialLength = length;
	}

	length = USB2_Attr_Get( msg->Function->FKT_ID, USB2Tag_Attr_PortPath, & identity->Port, sizeof( identity->Port ) );

	if ( ( length != sizeof( identity->Port ) ) || ( identity->Port.Depth > USB2_PORT_PATH_DEPTH ) )
	{
		identity->Port.Depth = 0;
	}
}

// --

static U32
Video_IdentityBytesMatch( const U8 *known, const U8 *found, U32 length )
{
	U32 index;

	for ( index = 0; index < length; index++ )
	{
		if ( known[index] != found[index] )
		{
			return ( FALSE );
		}
	}

	return ( TRUE );
}

// --

static U32
Video_IdentityMatch( const struct VideoIdentity *known, const struct VideoIdentity *found )
{
	S32 samePort;

	if ( ( known->VendorID != found->VendorID ) || ( known->DeviceID != found->DeviceID ) ||
		( known->InterfaceNr != found->InterfaceNr ) )
	{
		return ( 0 );
	}

	samePort = ( known->Port.Depth ) && ( known->Port.Depth == found->Port.Depth ) &&
		( known->Port.Controller == found->Port.Controller ) && ( known->Port.IsPCI == found->Port.IsPCI ) &&
		( Video_IdentityBytesMatch( known->Port.Ports, found->Port.Ports, known->Port.Depth ) );

	if ( ( known->SerialLength ) || ( found->SerialLength ) )
	{
		if ( ( known->SerialLength != found->SerialLength ) ||
			( ! Video_IdentityBytesMatch( known->Serial, found->Serial, known->SerialLength ) ) )
		{
			return ( 0 );
		}

		return ( samePort ? 2 : 1 );
	}

	return ( samePort ? 2 : 0 );
}

// --

static void
Video_AssignUnit( struct VideoUnit *unit, const struct VideoIdentity *identity )
{
	struct VideoBase *base;
	struct VideoSlot *slot;

	U32 index;
	U32 match;
	U32 bestMatch;
	U32 matching;
	U32 unused;
	U32 reusable;

	base = unit->Base;
	bestMatch = 0;
	matching = VIDEO_MAX_UNITS;
	unused = VIDEO_MAX_UNITS;
	reusable = VIDEO_MAX_UNITS;

	ObtainSemaphore( & base->Semaphore );

	for ( index = 0; index < VIDEO_MAX_UNITS; index++ )
	{
		slot = & base->Slots[index];

		if ( slot->Unit )
		{
			continue;
		}

		match = Video_IdentityMatch( & slot->Identity, identity );

		if ( match )
		{
			// Prefer the same port, then the open unit when otherwise tied.
			match = match * 2 + slot->Open;

			if ( match > bestMatch )
			{
				bestMatch = match;
				matching = index;
			}
		}

		if ( ! slot->Open )
		{
			if ( reusable == VIDEO_MAX_UNITS )
			{
				reusable = index;
			}

			if ( ( unused == VIDEO_MAX_UNITS ) && ( ! slot->Identity.SerialLength ) && ( ! slot->Identity.Port.Depth ) )
			{
				unused = index;
			}
		}
	}

	index = matching;

	if ( index == VIDEO_MAX_UNITS )
	{
		index = ( unused < VIDEO_MAX_UNITS ) ? unused : reusable;
	}

	if ( index < VIDEO_MAX_UNITS )
	{
		slot = & base->Slots[index];
		slot->Identity.Port = identity->Port;
		slot->Identity.VendorID = identity->VendorID;
		slot->Identity.DeviceID = identity->DeviceID;
		slot->Identity.InterfaceNr = identity->InterfaceNr;
		slot->Identity.SerialLength = identity->SerialLength;

		CopyMem( (PTR) identity->Serial, slot->Identity.Serial, identity->SerialLength );

		slot->Unit = unit;
		unit->UnitNr = index;
	}

	ReleaseSemaphore( & base->Semaphore );
}

// --

SEC_CODE struct VideoUnit *Video_Alloc( struct VideoContext *ctx, struct USB2_DriverMessage *msg )
{
struct VideoUnit *unit;
struct VideoBase *base;
struct VideoMode mode;
struct VideoIdentity identity;

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


	Video_GetIdentity( unit, & identity );

	Video_AssignUnit( unit, & identity );

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
