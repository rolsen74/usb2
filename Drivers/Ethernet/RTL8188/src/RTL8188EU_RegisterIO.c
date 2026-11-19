
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define RTL8188EU_REQ_REGS 0x05

// --

S32
RTL8188EU_RegisterIO( struct USB2_EPResource *control, U32 command, U16 address, U8 *data, U16 length )
{
	struct USB2_IORequest *ioreq;
	struct USB2_SetupData *setup;

	if ( ( ! control ) || ( ! control->IORequests ) || ( ! control->IORequests[0] ) ||
		( ! control->SetupData ) )
	{
		return ( FALSE );
	}

	ioreq = control->IORequests[0];
	setup = control->SetupData;
	setup->RequestType = REQTYPE_Vendor | REQTYPE_Device |
		( command == CMD_READ ? REQTYPE_Read : REQTYPE_Write );
	setup->RequestCode = RTL8188EU_REQ_REGS;
	setup->Value = LE_SWAP16( address );
	setup->Index = 0;
	setup->Length = LE_SWAP16( length );
	ioreq->io_Command = command;
	ioreq->io_Data = data;
	ioreq->io_Length = length;
	ioreq->io_SetupData = setup;
	ioreq->io_SetupLength = sizeof( *setup );
	ioreq->io_TimeOut = 1000000;

	DoIO( (PTR) ioreq );

	ioreq->io_Data = NULL;

	if ( ( ioreq->io_Error ) || ( ioreq->io_Actual != length ) )
	{
		USBERROR( "RTL8188EU: USB register %s 0x%04lx failed, error %ld, transferred %lu/%lu bytes",
			command == CMD_READ ? "read" : "write", (U32) address, (S32) ioreq->io_Error,
			ioreq->io_Actual, (U32) length );

		return ( FALSE );
	}

	return ( TRUE );
}

// --
