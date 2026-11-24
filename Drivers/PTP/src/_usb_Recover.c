
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

#define PTP_USB_DEVICE_RESET 0x66

// --

S32
_usb_Recover( struct USB_Struct *us )
{
	struct USB2_EPResource *control;
	struct USB2_IORequest *io;
	struct USB2_SetupData *setup;
	struct USB2IFace *usb;
	struct CommandMessage request;

	usb = us->us_StartMsg->IUSB2;
	control = us->us_Register->Res_Control;
	io = control->IORequests[0];
	setup = control->SetupData;

	_usb_Loop_Stack( us );

	if ( us->us_Detached )
	{
		return ( FALSE );
	}

	// Still Image Capture Device Reset closes the out-of-sync session.
	setup->RequestType = REQTYPE_Write | REQTYPE_Class | REQTYPE_Interface;
	setup->RequestCode = PTP_USB_DEVICE_RESET;
	setup->Value = LE_SWAP16( 0 );
	setup->Index = LE_SWAP16( us->us_StartMsg->InterfaceDescriptor->InterfaceNumber );
	setup->Length = LE_SWAP16( 0 );
	io->io_Command = CMD_WRITE;
	io->io_Length = 0;
	io->io_Data = NULL;
	io->io_TimeOut = 1000000;

	DoIO( (PTR)io );

	if ( io->io_Error == USB2Err_Device_Detached )
	{
		us->us_Detached = TRUE;
	}

	if ( io->io_Error )
	{
		return ( FALSE );
	}

	us->us_Session = FALSE;

	if ( ( ! usb->USB2_EPRes_Destall( us->us_Res_BulkIn ) ) ||
		( ! usb->USB2_EPRes_Destall( us->us_Res_BulkOut ) ) )
	{
		return ( FALSE );
	}

	_usb_Loop_Stack( us );

	if ( us->us_Detached )
	{
		return ( FALSE );
	}

	us->us_TransactionID = 0;
	us->us_TransportFailed = FALSE;

	memset( &request, 0, sizeof( request ) );

	request.cm_Operation = PTP_Open_Session;
	request.cm_ParameterCount = 1;
	request.cm_Parameters[0] = 1;

	_usb_Transaction( us, &request );

	if ( request.cm_Error )
	{
		return ( FALSE );
	}

	us->us_Session = TRUE;
	us->us_TransactionID = 1;

	return ( TRUE );
}

// --
