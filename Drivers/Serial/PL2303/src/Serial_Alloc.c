/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE struct SerialUnit *Serial_Alloc( struct SerialContext *ctx, struct USB2_DriverMessage *msg )
{
struct SerialBase *base;
struct SerialUnit *unit;

U32 ifcnr;
U32 index;

	base = ctx->Device;
	unit = AllocVecTags( sizeof( struct SerialUnit ), AVT_Clear, TRUE, TAG_END );

	if ( ! unit )
	{
		return ( NULL );
	}

	unit->Context = ctx;
	unit->Base = base;
	unit->StartMsg = msg;
	unit->UnitNr = SERIAL_MAX_UNITS;
	NewList( & unit->ReadList );
	NewList( & unit->WriteList );

	if ( ! ( unit->BeginPort = AllocSysObjectTags( ASOT_PORT, TAG_END ) ) )
	{
		goto bailout;
	}

	if ( ! ( unit->AbortPort = AllocSysObjectTags( ASOT_PORT, TAG_END ) ) )
	{
		goto bailout;
	}

	unit->Register = USB2_Reg_RegisterTags(
		USB2Tag_Reg_DriverMessage, msg,
		USB2Tag_Reg_Title, "PL2303 Serial",
		TAG_END
	);

	if ( ! unit->Register )
	{
		goto bailout;
	}

	unit->Control = unit->Register->Res_Control;
	ifcnr = msg->InterfaceDescriptor->InterfaceNumber;

	unit->BulkIn = USB2_EPRes_ObtainTags( unit->Register,
		USB2Tag_EPRes_InterfaceNr, ifcnr,
		USB2Tag_EPRes_EPType, EPATT_Type_Bulk,
		USB2Tag_EPRes_EPDirection, EPADR_Dir_In,
		USB2Tag_EPRes_AllowShortPackets, TRUE,
		USB2Tag_EPRes_BufferSize, USB2Val_BufferSize_MaxPacketSize,
		TAG_END
	);

	if ( ! unit->BulkIn )
	{
		goto bailout;
	}

	unit->BulkOut = USB2_EPRes_ObtainTags( unit->Register,
		USB2Tag_EPRes_InterfaceNr, ifcnr,
		USB2Tag_EPRes_EPType, EPATT_Type_Bulk,
		USB2Tag_EPRes_EPDirection, EPADR_Dir_Out,
		TAG_END
	);

	if ( ! unit->BulkOut )
	{
		goto bailout;
	}

	unit->Interrupt = USB2_EPRes_ObtainTags( unit->Register,
		USB2Tag_EPRes_InterfaceNr, ifcnr,
		USB2Tag_EPRes_EPType, EPATT_Type_Interrupt,
		USB2Tag_EPRes_EPDirection, EPADR_Dir_In,
		USB2Tag_EPRes_BufferSize, USB2Val_BufferSize_MaxPacketSize,
		TAG_END
	);

	if ( ! unit->Interrupt )
	{
		goto bailout;
	}

	if ( ! Serial_Startup( ctx, unit ) )
	{
		goto bailout;
	}

	ObtainSemaphore( & base->Semaphore );

	for( index = 0; index < SERIAL_MAX_UNITS; index++ )
	{
		if ( ! base->Units[index] )
		{
			base->Units[index] = unit;
			unit->UnitNr = index;
			break;
		}
	}

	ReleaseSemaphore( & base->Semaphore );

	if ( unit->UnitNr == SERIAL_MAX_UNITS )
	{
		goto bailout;
	}

	return ( unit );

bailout:
	Serial_Free( ctx, unit );

	return ( NULL );
}

// --
