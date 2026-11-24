
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
__Boot_Keyboard_Data( struct USBBase *usbbase, struct HID_ReportNode *rn, struct RealRequest *ioreq )
{
	struct USB2_BootKeyboard *report;

	U8 keys[32];
	U32 index;
	U32 usage;

	report = ioreq->req_Public.io_Data;

	MEM_SET( keys, 0, sizeof( keys ) );

	keys[0xe0 / 8] = report->Modifier;

	for ( index = 0; index < sizeof( report->Keycode ); index++ )
	{
		usage = report->Keycode[index];

		if ( ( usage >= 1 ) && ( usage <= 3 ) )
		{
			return;
		}

		if ( usage )
		{
			keys[usage / 8] |= 1U << ( usage & 7 );
		}
	}

	__Keyboard_Keys( usbbase, rn, keys );
}

// --
