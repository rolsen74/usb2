
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
BTUSB_Open( struct BTUSB *usb, struct USB2_DriverMessage *message, U8 endpoint, U8 recipient )
{
	struct USB2_EPResource *control;

	usb->USB2 = message->IUSB2;
	usb->InterfaceNumber = message->Interface->IFC_Num;
	usb->CommandRecipient = recipient;
	usb->EventPending = BTUSB_EVENT_IDLE;
	usb->PrefetchEvents = FALSE;
	usb->TimerPort = NULL;
	usb->TimerRequest = NULL;
	usb->Timer = NULL;
	usb->Cancel = NULL;
	usb->RequestSignal = 0;
	usb->EventOffset = 0;
	usb->EventBytes = 0;
	usb->FrameUsed = 0;
	usb->FrameLength = 2;
	usb->ResponsePending = FALSE;
	usb->UncertainResponse = FALSE;
	usb->Detached = FALSE;
	usb->Stopping = FALSE;
	usb->CheckingReadiness = FALSE;
	usb->Events = NULL;
	usb->ACLIn = NULL;
	usb->ACLOut = NULL;

	usb->Registration = usb->USB2->USB2_Reg_RegisterTags( USB2Tag_Reg_DriverMessage, message,
		USB2Tag_Reg_Title, "Bluetooth USB HCI", TAG_END );

	if ( ! usb->Registration )
	{
		return ( FALSE );
	}

	DebugPrintF( "Bluetooth USB: opening HCI endpoints for %04lx:%04lx if %lu, event EP $%02lx\n",
		(U32) message->VendorID, (U32) message->DeviceID, (U32) usb->InterfaceNumber, (U32) endpoint );

	control = usb->Registration->Res_Control;

	if ( ( ! control ) || ( ! control->SetupData ) || ( ! control->IORequests ) || ( ! control->IORequests[0] ) )
	{
		return ( FALSE );
	}

	if ( ! usb->USB2->USB2_Ifc_SetAlt( usb->Registration, message->Interface, 0 ) )
	{
		DebugPrintF( "Bluetooth USB: could not select legacy HCI alternate 0\n" );

		return ( FALSE );
	}

	if ( ! BTUSB_OpenTimer( usb ) )
	{
		DebugPrintF( "Bluetooth USB: could not open HCI timer\n" );

		return ( FALSE );
	}

	usb->EventEndpoint = endpoint;

	usb->Events = usb->USB2->USB2_EPRes_ObtainTags( usb->Registration,
		USB2Tag_EPRes_InterfaceNr, usb->InterfaceNumber,
		USB2Tag_EPRes_EndPointNr, endpoint, USB2Tag_EPRes_EPType, EPATT_Type_Interrupt,
		USB2Tag_EPRes_NrOfIORequest, 1,
		USB2Tag_EPRes_BufferSize, USB2Val_BufferSize_MaxPacketSize, USB2Tag_EPRes_AllowShortPackets, TRUE,
		USB2Tag_EPRes_TimeOut, HCI_USB_TIMEOUT, TAG_END );

	return ( ( usb->Events ) && ( usb->Events->BufferSize ) );
}

// --
