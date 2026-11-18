
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

SEC_RODATA static const U16 Tablet_ButtonCodes[] =
{
	IECODE_LBUTTON, IECODE_RBUTTON, IECODE_MBUTTON
};

SEC_RODATA static const U16 Tablet_ButtonQualifiers[] =
{
	IEQUALIFIER_LEFTBUTTON, IEQUALIFIER_RBUTTON, IEQUALIFIER_MIDBUTTON
};

// --

SEC_CODE void
Tablet_Buttons ( struct USBBase * usbbase, struct HID_ReportNode * rn, U8 buttons )
{
	U8 changed;
	U8 bit;
	U16 code;
	U32 index;

	changed = rn->Tablet_Buttons ^ buttons;
	rn->Tablet_Buttons = buttons;

	for ( index = 0; index < 3; index++ )
	{
		bit = 1U << index;

		if ( ! ( changed & bit ) )
		{
			continue;
		}

		code = Tablet_ButtonCodes[index];

		if ( buttons & bit )
		{
			usbbase->usb_HID_Qualifiers |= Tablet_ButtonQualifiers[index];
		}
		else
		{
			usbbase->usb_HID_Qualifiers &= ~Tablet_ButtonQualifiers[index];
			code |= IECODE_UP_PREFIX;
		}

		Input_Mouse_Button( usbbase, rn, code );
	}
}

// --
