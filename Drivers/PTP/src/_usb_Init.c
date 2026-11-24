
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

U32
_usb_Init( struct USB_Struct *us )
{
	struct CommandMessage request;
	struct USB2IFace *usb;

	U32 interface;

	usb = us->us_StartMsg->IUSB2;
	interface = us->us_StartMsg->InterfaceDescriptor->InterfaceNumber;

	InitSemaphore( & us->us_CameraNode.cin_Gate );

	us->us_CmdMsgPort = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! us->us_CmdMsgPort )
	{
		return ( FALSE );
	}

	us->us_Register = usb->USB2_Reg_RegisterTags(
		USB2Tag_Reg_DriverMessage, us->us_StartMsg,
		USB2Tag_Reg_Title, "PTP / MTP",
		TAG_END );

	if ( ! us->us_Register )
	{
		return ( FALSE );
	}

	if ( us->us_StartMsg->InterfaceDescriptor->InterfaceClass != USBCLASS_Image )
	{
		if ( ! _usb_MTP_Interface( us ) )
		{
			return ( FALSE );
		}

		us->us_CameraNode.cin_MTP = TRUE;
	}

	us->us_Res_BulkIn = usb->USB2_EPRes_ObtainTags( us->us_Register,
		USB2Tag_EPRes_InterfaceNr, interface,
		USB2Tag_EPRes_BufferSize, USB2Val_BufferSize_MaxPacketSize_16,
		USB2Tag_EPRes_AllowShortPackets, TRUE,
		USB2Tag_EPRes_EPDirection, EPADR_Dir_In,
		USB2Tag_EPRes_EPType, EPATT_Type_Bulk,
		TAG_END );

	if ( ! us->us_Res_BulkIn )
	{
		return ( FALSE );
	}

	us->us_Res_BulkOut = usb->USB2_EPRes_ObtainTags( us->us_Register,
		USB2Tag_EPRes_InterfaceNr, interface,
		USB2Tag_EPRes_BufferSize, USB2Val_BufferSize_MaxPacketSize_16,
		USB2Tag_EPRes_EPDirection, EPADR_Dir_Out,
		USB2Tag_EPRes_EPType, EPATT_Type_Bulk,
		TAG_END );

	if ( ! us->us_Res_BulkOut )
	{
		return ( FALSE );
	}

	us->us_Res_Events = usb->USB2_EPRes_ObtainTags( us->us_Register,
		USB2Tag_EPRes_InterfaceNr, interface,
		USB2Tag_EPRes_BufferSize, USB2Val_BufferSize_MaxPacketSize,
		USB2Tag_EPRes_AllowShortPackets, TRUE,
		USB2Tag_EPRes_EPDirection, EPADR_Dir_In,
		USB2Tag_EPRes_EPType, EPATT_Type_Interrupt,
		TAG_END );

	memset( & request, 0, sizeof( request ) );

	request.cm_Operation = PTP_Open_Session;
	request.cm_ParameterCount = 1;
	request.cm_Parameters[0] = 1;
	us->us_Running = TRUE;

	_usb_Transaction( us, & request );

	if ( request.cm_Error )
	{
		return ( FALSE );
	}

	us->us_Session = TRUE;
	us->us_TransactionID = 1;

	_usb_Query_Device_Info( us );

	if ( us->us_TransportFailed )
	{
		return ( FALSE );
	}

	us->us_CameraNode.cin_ID = us->us_Register->FKT_ID;
	us->us_CameraNode.cin_MsgPort = us->us_CmdMsgPort;
	us->us_CameraNode.cin_Apple = ( us->us_StartMsg->VendorID == 0x05ac );

	return ( TRUE );
}

// --
