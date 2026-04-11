
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
// Send Amiga Raw Key to System

SEC_CODE void Input_Raw_Key( struct USBBase *usbbase, struct HID_ReportNode *rn, U16 rawkey )
{
struct InputEvent *Event;
struct IOStdReq *ioreq;
U16 key;

	// --

	TASK_NAME_ENTER( "Input_Raw_Key" );

	// --

	if ( rawkey )
	{
		Event = & rn->Input_Event;
		Event->ie_NextEvent		= NULL;
		Event->ie_Class			= IECLASS_RAWKEY;
		Event->ie_SubClass		= 0;
		Event->ie_Code			= rawkey;
		Event->ie_Qualifier		= usbbase->usb_HID_Qualifiers;
		Event->ie_Prev1DownCode	= usbbase->usb_HID_Prev_Code[0];
		Event->ie_Prev1DownQual	= usbbase->usb_HID_Prev_Qual[0];
		Event->ie_Prev2DownCode	= usbbase->usb_HID_Prev_Code[1];
		Event->ie_Prev2DownQual	= usbbase->usb_HID_Prev_Qual[1];

		ioreq = & rn->Input_IOReq;
		ioreq->io_Command		= IND_ADDEVENT;
		ioreq->io_Data			= Event;
		ioreq->io_Length		= sizeof( struct InputEvent );

		IO_DO( ioreq );

		// --
		// Amiga Raw KeyCodes
		//  $60 = LShift
		//  $61 = RShift
		//  $62 = Caps Lock
		//  $63 = Ctrl
		//  $64 = LAlt
		//  $65 = RAlt
		//  $66 = LAmiga / LCommand
		//  $67 = RAmiga / RCommand
		//	$68 = Left Mouse button (not used)
		//	$69 = Right Mouse button (not used)
		//	$6A = Middle Mouse button (not used)
		key = ( rawkey & ~IECODE_UP_PREFIX );

		if (( key < 0x60 ) && ( key > 0x6A ))
		{
			usbbase->usb_HID_Prev_Code[1] = usbbase->usb_HID_Prev_Code[0];
			usbbase->usb_HID_Prev_Code[0] = rawkey;
			usbbase->usb_HID_Prev_Qual[1] = usbbase->usb_HID_Prev_Qual[0];
			usbbase->usb_HID_Prev_Qual[0] = usbbase->usb_HID_Qualifiers;
		}
	}

	// -- Check for Reboot qualifiers

	if ((( IEQUALIFIER_CONTROL | IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND ) & usbbase->usb_HID_Qualifiers )
	==	(( IEQUALIFIER_CONTROL | IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND )))
	{
		MISC_COLDREBOOT();
	}

	if ((( IEQUALIFIER_CONTROL | IEQUALIFIER_LALT | IEQUALIFIER_RALT ) & usbbase->usb_HID_Qualifiers )
	==	(( IEQUALIFIER_CONTROL | IEQUALIFIER_LALT | IEQUALIFIER_RALT )))
	{
		MISC_ICECOLDREBOOT();
	}

	// --

	TASK_NAME_LEAVE();
}
