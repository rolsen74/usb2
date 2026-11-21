
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
BTUSB_Open( struct BTUSB *usb, struct USB2_DriverMessage *message, U8 endpoint )
{
	usb->USB2 = message->IUSB2;
	usb->InterfaceNumber = message->Interface->IFC_Num;
	usb->EventPending = FALSE;
	usb->Events = NULL;

	usb->Registration = usb->USB2->USB2_Reg_RegisterTags( USB2Tag_Reg_DriverMessage, message,
		USB2Tag_Reg_Title, "Bluetooth USB HCI", TAG_END );

	if ( ! usb->Registration )
	{
		return ( FALSE );
	}

	usb->Events = usb->USB2->USB2_EPRes_ObtainTags( usb->Registration,
		USB2Tag_EPRes_InterfaceNr, usb->InterfaceNumber,
		USB2Tag_EPRes_EndPointNr, endpoint, USB2Tag_EPRes_NrOfIORequest, 1,
		USB2Tag_EPRes_BufferSize, USB2Val_BufferSize_MaxPacketSize, USB2Tag_EPRes_AllowShortPackets, TRUE,
		USB2Tag_EPRes_TimeOut, HCI_USB_TIMEOUT, TAG_END );

	return ( ( usb->Events ) && ( usb->Events->BufferSize ) );
}

// --
