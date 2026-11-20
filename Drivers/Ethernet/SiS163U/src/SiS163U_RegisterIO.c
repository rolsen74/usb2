
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define SIS163U_REGISTER_SPACE 0x4000
#define SIS163U_REGISTER_LENGTH 4

// --

S32
SiS163U_RegisterIO( struct USB2_EPResource *control, U16 command, U8 requestCode, U16 address, U8 *data )
{
	struct USB2_IORequest *request;
	struct USB2_SetupData *setup;

	S32 error;

	if ( ( ! control ) || ( ! control->SetupData ) || ( ! control->IORequests ) || ( ! control->IORequests[0] ) )
	{
		return ( USB2Err_Stack_InvalidStructure );
	}

	request = control->IORequests[0];
	setup = control->SetupData;
	// SiS uses a class/interface control request and encodes the register in wIndex.
	setup->RequestType = REQTYPE_Class | REQTYPE_Interface | ( command == CMD_READ ? REQTYPE_Read : REQTYPE_Write );
	setup->RequestCode = requestCode;
	setup->Value = 0;
	setup->Index = LE_SWAP16( address | SIS163U_REGISTER_SPACE );
	setup->Length = LE_SWAP16( SIS163U_REGISTER_LENGTH );
	request->io_Command = command;
	request->io_SetupData = setup;
	request->io_SetupLength = sizeof( *setup );
	request->io_Data = data;
	request->io_Length = SIS163U_REGISTER_LENGTH;
	request->io_AllowShortPackets = FALSE;
	request->io_TimeOut = 1000000;

	error = DoIO( (PTR) request );

	request->io_Data = NULL;

	if ( error )
	{
		return ( error );
	}

	if ( request->io_Actual != SIS163U_REGISTER_LENGTH )
	{
		return ( USB2Err_Host_RuntPacket );
	}

	return ( 0 );
}

// --
