
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Controller.h"

// --

int
RTL8152_Control( struct devUnit *unit, U32 command, U16 address, U16 index, U8 *data, U16 length )
{
	struct USB2_SetupData *setup;
	struct USB2_IORequest *ioreq;

	if ( unit->unit_USBControlError )
	{
		return ( unit->unit_USBControlError );
	}

	ioreq = unit->unit_USBControlIOReq;
	setup = unit->unit_USBRegister->Res_Control->SetupData;
	setup->RequestType = REQTYPE_Vendor | REQTYPE_Device |
		( command == CMD_READ ? REQTYPE_Read : REQTYPE_Write );
	setup->RequestCode = 0x05;
	setup->Value = LE_SWAP16( address );
	setup->Index = LE_SWAP16( index );
	setup->Length = LE_SWAP16( length );
	ioreq->io_Command = command;
	ioreq->io_Length = length;
	ioreq->io_Data = data;
	ioreq->io_SetupData = setup;
	ioreq->io_SetupLength = sizeof( *setup );
	ioreq->io_TimeOut = 1000000;

	IExec->DoIO( (PTR) ioreq );

	ioreq->io_Data = NULL;
	unit->unit_USBControlError = ioreq->io_Error ? ioreq->io_Error :
		( ioreq->io_Actual == length ? 0 : IOERR_BADLENGTH );

	return ( unit->unit_USBControlError );
}

// --
