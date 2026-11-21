
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

static S32
ReadString( struct USB2_EPResource *control, U8 index, U16 language, U8 *data, U16 length )
{
	struct USB2_IORequest *io;
	struct USB2_SetupData *setup;

	io = control->IORequests[0];
	setup = control->SetupData;
	setup->RequestType = REQTYPE_Read | REQTYPE_Standard | REQTYPE_Device;
	setup->RequestCode = REQCODE_Get_Descriptor;
	setup->Value = LE_SWAP16( ( DSCTYPE_String << 8 ) | index );
	setup->Index = LE_SWAP16( language );
	setup->Length = LE_SWAP16( length );
	io->io_Command = CMD_READ;
	io->io_Data = data;
	io->io_Length = length;
	io->io_SetupData = setup;
	io->io_SetupLength = sizeof( *setup );
	io->io_TimeOut = 500000;

	DoIO( (PTR) io );

	io->io_Data = NULL;

	return ( ( ! io->io_Error ) && ( io->io_Actual == length ) && ( data[1] == DSCTYPE_String ) );
}

// --

S32
_usb_MTP_Interface( struct USB_Struct *us )
{
	struct USB2_EPResource *control;

	U8 string[8];
	U8 index;
	U16 language;

	control = us->us_Register->Res_Control;
	index = us->us_StartMsg->InterfaceDescriptor->InterfaceStrNr;

	if ( ( ! index ) || ( ! control ) || ( ! control->SetupData ) ||
		( ! control->IORequests ) || ( ! control->IORequests[0] ) )
	{
		return ( FALSE );
	}

	if ( ( ! ReadString( control, 0, 0, string, 4 ) ) || ( string[0] < 4 ) )
	{
		return ( FALSE );
	}

	language = string[2] | ( (U16) string[3] << 8 );

	if ( ! ReadString( control, index, language, string, sizeof( string ) ) )
	{
		return ( FALSE );
	}

	return ( ( string[0] == sizeof( string ) ) && ( string[2] == 'M' ) && ( ! string[3] ) &&
		( string[4] == 'T' ) && ( ! string[5] ) && ( string[6] == 'P' ) && ( ! string[7] ) );
}

// --
