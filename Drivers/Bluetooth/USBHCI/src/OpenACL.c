
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
BTUSB_OpenACL( struct BTUSB *usb )
{
	usb->ACLIn = usb->USB2->USB2_EPRes_ObtainTags( usb->Registration,
		USB2Tag_EPRes_InterfaceNr, usb->InterfaceNumber, USB2Tag_EPRes_EPDirection, EPADR_Dir_In,
		USB2Tag_EPRes_EPType, EPATT_Type_Bulk, USB2Tag_EPRes_NrOfIORequest, 1,
		USB2Tag_EPRes_BufferSize, USB2Val_BufferSize_MaxPacketSize, USB2Tag_EPRes_AllowShortPackets, TRUE,
		USB2Tag_EPRes_TimeOut, HCI_USB_TIMEOUT, TAG_END );

	if ( ! usb->ACLIn )
	{
		return ( FALSE );
	}

	usb->ACLOut = usb->USB2->USB2_EPRes_ObtainTags( usb->Registration,
		USB2Tag_EPRes_InterfaceNr, usb->InterfaceNumber, USB2Tag_EPRes_EPDirection, EPADR_Dir_Out,
		USB2Tag_EPRes_EPType, EPATT_Type_Bulk, USB2Tag_EPRes_NrOfIORequest, 1,
		USB2Tag_EPRes_BufferSize, 1024, USB2Tag_EPRes_TimeOut, HCI_USB_TIMEOUT, TAG_END );

	if ( ! usb->ACLOut )
	{
		usb->USB2->USB2_EPRes_Release( usb->ACLIn );

		usb->ACLIn = NULL;
	}

	return ( usb->ACLOut != NULL );
}

// --
