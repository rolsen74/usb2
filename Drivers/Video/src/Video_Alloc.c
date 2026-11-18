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

	if ( ! Video_Parse( unit ) )
	{
		goto bailout;
	}

	unit->BeginPort = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! unit->BeginPort )
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

	if ( ! Video_Probe( unit ) )
	{
		goto bailout;
	}

	if ( unit->StreamAlt )
	{
		// Older stacks allocate shorter requests without isochronous results.
		if ( ( unit->Control->IORequests[0]->io_Message.mn_Length < sizeof( struct USB2_IORequest ) ) ||
			( ! Video_Select_Stream( unit, unit->PayloadSize ) ) ||
			( ! USB2_Ifc_SetAlt( unit->Register, msg->Interface, unit->StreamAlt ) ) )
		{
			goto bailout;
		}

		unit->StreamAltActive = TRUE;
	}

	unit->Stream = USB2_EPRes_ObtainTags( unit->Register,
		USB2Tag_EPRes_InterfaceNr, unit->InterfaceNr,
		USB2Tag_EPRes_EndPointNr, unit->EndPoint,
		USB2Tag_EPRes_BufferSize, unit->StreamAlt ? unit->StreamPacketSize * VIDEO_ISO_PACKETS : unit->PayloadSize,
		USB2Tag_EPRes_AllowShortPackets, TRUE,
		TAG_END
	);

	if ( ! unit->Stream )
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
