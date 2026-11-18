/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE struct AudioUnit *
Audio_Alloc( struct AudioContext *ctx, struct USB2_DriverMessage *msg )
{
struct AudioUnit *unit;
struct AudioBase *base;
struct USB2_EPResource *control;

U32 index;
U32 limit;

	base = ctx->Device;
	unit = AllocVecTags( sizeof( struct AudioUnit ), AVT_Clear, TRUE, TAG_END );

	if ( ! unit )
	{
		return ( NULL );
	}

	unit->Context = ctx;
	unit->Base = base;
	unit->StartMsg = msg;
	unit->UnitNr = AUDIO_MAX_UNITS;
	unit->InterfaceNr = msg->InterfaceDescriptor->InterfaceNumber;

	if ( ! Audio_Parse( unit ) )
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
		USB2Tag_Reg_Title, "USB Audio",
		TAG_END
	);

	if ( ! unit->Register )
	{
		goto bailout;
	}

	control = unit->Register->Res_Control;

	if ( ( ! control ) || ( ! control->IORequests ) || ( ! control->IORequests[0] ) ||
		( control->IORequests[0]->io_Message.mn_Length < sizeof( struct USB2_IORequest ) ) )
	{
		goto bailout;
	}

	if ( ! USB2_Ifc_SetAlt( unit->Register, msg->Interface, unit->AltNr ) )
	{
		goto bailout;
	}

	unit->ActiveAlt = TRUE;

	if ( ! Audio_Frequency( unit, unit->Format.SampleRate ) )
	{
		goto bailout;
	}

	unit->RateValid = TRUE;

	unit->Stream = USB2_EPRes_ObtainTags( unit->Register,
		USB2Tag_EPRes_InterfaceNr, unit->InterfaceNr,
		USB2Tag_EPRes_EndPointNr, unit->EndPoint,
		USB2Tag_EPRes_BufferSize, unit->PacketBytes * unit->PacketCount,
		USB2Tag_EPRes_AllowShortPackets, TRUE,
		TAG_END
	);

	if ( ! unit->Stream )
	{
		goto bailout;
	}

	ObtainSemaphore( & base->Semaphore );

	index = ( unit->EndPoint & EPADR_Dir_Mask ) ? 0 : AUDIO_PLAYBACK_UNIT_BASE;
	limit = index ? AUDIO_MAX_UNITS : AUDIO_PLAYBACK_UNIT_BASE;

	for( ; index < limit; index++ )
	{
		if ( ! base->Units[index] )
		{
			base->Units[index] = unit;
			unit->UnitNr = index;
			break;
		}
	}

	ReleaseSemaphore( & base->Semaphore );

	if ( unit->UnitNr == AUDIO_MAX_UNITS )
	{
		goto bailout;
	}

	return ( unit );

bailout:
	Audio_Free( unit );

	return ( NULL );
}

// --
