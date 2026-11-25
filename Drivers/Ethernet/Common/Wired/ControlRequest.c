
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

S32
Wired_ControlRequest( struct devUnit *unit, U8 type, U8 request, U16 value, U16 index, U8 *data, U16 length )
{
	struct USB2_SetupData *setup;
	struct USB2_IORequest *ioreq;

	ioreq = unit->unit_USBControlIOReq;
	setup = unit->unit_USBRegister->Res_Control->SetupData;
	setup->RequestType = type;
	setup->RequestCode = request;
	setup->Value = LE_SWAP16( value );
	setup->Index = LE_SWAP16( index );
	setup->Length = LE_SWAP16( length );
	ioreq->io_Command = ( type & REQTYPE_Read ) ? CMD_READ : CMD_WRITE;
	ioreq->io_Length = length;
	ioreq->io_Data = data;
	ioreq->io_SetupData = setup;
	ioreq->io_SetupLength = sizeof( *setup );
	ioreq->io_TimeOut = 1000000;

	IExec->DoIO( (PTR) ioreq );

	ioreq->io_Data = NULL;

	return ( ioreq->io_Error ? ioreq->io_Error :
		( ioreq->io_Actual == length ? 0 : IOERR_BADLENGTH ) );
}

// --
