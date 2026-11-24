
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "HID.h"

// --

SEC_CODE void
__Boot_Mouse_Data( struct USBBase *usbbase, struct HID_ReportNode *rn, struct RealRequest *ioreq )
{
	struct USB2_BootMouse *data;

	U32 index;

	data = ioreq->req_Public.io_Data;

	for ( index = 0; index < 5; index++ )
	{
		HID_Mouse_Button( usbbase, rn, index, data->Buttons & ( 1U << index ) );
	}

	Input_Mouse_Move( usbbase, rn, data->X, data->Y );
}

// --
