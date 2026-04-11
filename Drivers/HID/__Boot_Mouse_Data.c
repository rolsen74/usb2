
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

struct hhh
{
	U16		USBButton;
	U16		IEQual;
	U16		IECode;
};

SEC_RWDATA static struct hhh ooo[] =
{
{ BOOTMOUSE_BUTTONS_LEFT,		IEQUALIFIER_LEFTBUTTON,		IECODE_LBUTTON },
{ BOOTMOUSE_BUTTONS_RIGHT,		IEQUALIFIER_RBUTTON,		IECODE_RBUTTON },
{ BOOTMOUSE_BUTTONS_MIDDLE,		IEQUALIFIER_MIDBUTTON,		IECODE_MBUTTON },
{ BOOTMOUSE_BUTTONS_4TH,		0,							IECODE_4TH_BUTTON },
{ BOOTMOUSE_BUTTONS_5TH,		0,							IECODE_5TH_BUTTON },
{ 0, 0, 0 }
};

SEC_CODE static void __Buttons( struct USBBase *usbbase, struct HID_ReportNode *rn, struct USB2_BootMouse *data )
{
U16 code;
U16 new;	// new Button mask
U16 old;	// old Button mask
U16 chg;	// Change Mask
U32 pos;

	TASK_NAME_ENTER( "__Buttons" );

	old = rn->Boot_Mouse_Buttons;
	new = data->Buttons;
	chg = ( new ^ old ) & BOOTMOUSE_BUTTONS_MASK;

	if ( chg )
	{
		pos = 0;

		while( TRUE )
		{
			// End of Struct ?
			if ( ! ooo[pos].USBButton )
			{
				break;
			}

			// Button Change ?
			if ( ooo[pos].USBButton & chg )
			{
				if ( ooo[pos].USBButton & new )
				{
					usbbase->usb_HID_Qualifiers |= ooo[pos].IEQual;
					code = ooo[pos].IECode;
				}
				else
				{
					usbbase->usb_HID_Qualifiers &= ~ooo[pos].IEQual;
					code = ooo[pos].IECode | IECODE_UP_PREFIX;
				}

				Input_Mouse_Button( usbbase, rn, code );
			}

			// Next struct Entry
			pos++;
		}

		// --
		// Store Current Mask
		rn->Boot_Mouse_Buttons = new;
	}

	TASK_NAME_LEAVE();
}

// --

SEC_CODE void __Boot_Mouse_Data( 
	struct USBBase *usbbase, 
	struct HID_ReportNode *rn, 
	struct RealRequest *ioreq )
{
struct USB2_BootMouse *data;

	TASK_NAME_ENTER( "__Boot_Mouse_Data" );

	data = (PTR) ioreq->req_Public.io_Data;

	#if 0
	USBDEBUG( "Buttons . : $%02lx", (U32) data->Buttons );
	USBDEBUG( "X ....... : %ld", (S32) data->X );
	USBDEBUG( "Y ....... : %ld", (S32) data->Y );
	#endif

	// -- Check for Button mask

	__Buttons( usbbase, rn, data );

	// -- Check for Movements

	Input_Mouse_Move( usbbase, rn, data->X, data->Y );

	TASK_NAME_LEAVE();
}

// --
