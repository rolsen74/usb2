
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

SEC_RODATA static const U16 ButtonCodes[] =
{
	IECODE_LBUTTON, IECODE_RBUTTON, IECODE_MBUTTON, IECODE_4TH_BUTTON, IECODE_5TH_BUTTON
};

SEC_RODATA static const U16 ButtonQualifiers[] =
{
	IEQUALIFIER_LEFTBUTTON, IEQUALIFIER_RBUTTON, IEQUALIFIER_MIDBUTTON, 0, 0
};

// --

SEC_CODE void
HID_Mouse_Button( struct USBBase *usbbase, struct HID_ReportNode *rn, U32 index, U32 down )
{
	U32 bit;

	bit = 1U << index;

	if ( ( ( rn->Buttons & bit ) != 0 ) == ( down != 0 ) )
	{
		return;
	}

	rn->Buttons ^= bit;

	SEMAPHORE_OBTAIN( &usbbase->usb_HID_Semaphore );

	if ( HID_Hold_Input( usbbase, 128 + index, ButtonQualifiers[index], down ) )
	{
		Input_Mouse_Button( usbbase, rn, ButtonCodes[index] | ( down ? 0 : IECODE_UP_PREFIX ) );
	}

	SEMAPHORE_RELEASE( &usbbase->usb_HID_Semaphore );
}

// --
