
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define RTL8821CU_REQ_REGS 0x05
#define RTL8821CU_USB_REG_SYNC 0x04e0

// --

static S32
ControlTransfer( struct USB2_EPResource *control, U16 command, U16 address, PTR data, U16 length )
{
	struct USB2_IORequest *ioreq;
	struct USB2_SetupData *setup;

	S32 error;

	ioreq = control->IORequests[0];
	setup = control->SetupData;
	setup->RequestType = REQTYPE_Vendor | REQTYPE_Device |
		( ( command == CMD_READ ) ? REQTYPE_Read : REQTYPE_Write );
	setup->RequestCode = RTL8821CU_REQ_REGS;
	setup->Value = LE_SWAP16( address );
	setup->Index = 0;
	setup->Length = LE_SWAP16( length );
	ioreq->io_Command = command;
	ioreq->io_Data = data;
	ioreq->io_Length = length;
	ioreq->io_SetupData = setup;
	ioreq->io_SetupLength = sizeof( *setup );
	ioreq->io_TimeOut = 1000000;

	error = DoIO( (PTR) ioreq );

	ioreq->io_Data = NULL;

	if ( ( error ) || ( ioreq->io_Actual != length ) )
	{
		DebugPrintF( "RTL8821CU: register $%04lx failed, error %ld, transferred %lu/%lu\n",
			(U32) address, error, ioreq->io_Actual, (U32) length );

		return ( FALSE );
	}

	return ( TRUE );
}

// --

S32
RTL8821CU_RegisterIO( struct USB2_EPResource *control, U16 command, U16 address, PTR data, U16 length )
{
	if ( ( ! control ) || ( ! control->IORequests ) || ( ! control->IORequests[0] ) ||
		( ! control->SetupData ) )
	{
		return ( FALSE );
	}

	if ( ! ControlTransfer( control, command, address, data, length ) )
	{
		return ( FALSE );
	}

	// Accesses to the always-on register banks need a follow-up byte write.
	if ( ( address <= 0x00ff ) || ( ( address >= 0x1000 ) && ( address <= 0x10ff ) ) )
	{
		return ( ControlTransfer( control, CMD_WRITE, RTL8821CU_USB_REG_SYNC, data, 1 ) );
	}

	return ( TRUE );
}

// --
