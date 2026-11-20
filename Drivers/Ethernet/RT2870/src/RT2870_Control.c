
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
RT2870_Control( struct USB2_EPResource *control, U8 direction, U8 command, U16 value, U16 address, PTR data, U16 length )
{
	struct USB2_IORequest *request;
	struct USB2_SetupData *setup;

	S32 error;

	request = control->IORequests[0];
	setup = control->SetupData;

	setup->RequestType = direction | REQTYPE_Vendor | REQTYPE_Device;
	setup->RequestCode = command;
	setup->Value = LE_SWAP16( value );
	setup->Index = LE_SWAP16( address );
	setup->Length = LE_SWAP16( length );
	request->io_Command = direction == REQTYPE_Read ? CMD_READ : CMD_WRITE;
	request->io_SetupData = setup;
	request->io_SetupLength = sizeof( *setup );
	request->io_Data = data;
	request->io_Length = length;
	request->io_AllowShortPackets = FALSE;
	request->io_TimeOut = 1000000;

	error = DoIO( (PTR) request );

	request->io_Data = NULL;

	if ( ( ! error ) && ( request->io_Actual != length ) )
	{
		error = USB2Err_Host_RuntPacket;
	}

	if ( error )
	{
		USBERROR( "RT2870: USB %s request $%02lx value $%04lx index $%04lx failed, error %ld, transferred %lu/%lu",
			direction == REQTYPE_Read ? "read" : "write", (U32) command, (U32) value, (U32) address,
			error, request->io_Actual, (U32) length );
	}

	return ( error );
}

// --
