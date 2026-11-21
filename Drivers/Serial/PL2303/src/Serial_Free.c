/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Serial_Free( struct SerialContext *ctx, struct SerialUnit *unit )
{
struct IOExtSer *ioreq;

	if ( ! unit )
	{
		return;
	}

	if ( unit->PortUnit )
	{
		unit->PortInterface->Unregister( unit->PortUnit );
	}

	if ( unit->PortInterface )
	{
		DropInterface( (PTR) unit->PortInterface );
	}

	if ( unit->PortRequest.io_Device )
	{
		CloseDevice( & unit->PortRequest );
	}

	if ( unit->ReadPending )
	{
		Serial_IO_Stop( ctx, unit->BulkIn->IORequests[0] );
		unit->ReadPending = FALSE;
	}

	if ( unit->WritePending )
	{
		Serial_IO_Stop( ctx, unit->BulkOut->IORequests[0] );
		unit->WritePending = FALSE;
	}

	if ( unit->InterruptPending )
	{
		Serial_IO_Stop( ctx, unit->Interrupt->IORequests[0] );
		unit->InterruptPending = FALSE;
	}

	Serial_Flush( ctx, unit );

	if ( unit->BeginPort )
	{
		while(( ioreq = (PTR) GetMsg( unit->BeginPort ) ))
		{
			ioreq->IOSer.io_Error = IOERR_ABORTED;
			Serial_Reply( ctx, ioreq );
		}
	}

	if ( unit->WriteActive )
	{
		unit->WriteActive->IOSer.io_Error = IOERR_ABORTED;
		Serial_Reply( ctx, unit->WriteActive );
		unit->WriteActive = NULL;
	}

	if ( unit->Register )
	{
		if ( unit->DataAlt )
		{
			USB2_Ifc_SetAlt( unit->Register, unit->DataInterface, 0 );
		}

		USB2_Reg_Unregister( unit->Register );
		unit->Register = NULL;
	}

	if ( unit->AbortPort )
	{
		FreeSysObject( ASOT_PORT, unit->AbortPort );
		unit->AbortPort = NULL;
	}

	if ( unit->BeginPort )
	{
		FreeSysObject( ASOT_PORT, unit->BeginPort );
		unit->BeginPort = NULL;
	}

	FreeVec( unit );
}

// --
