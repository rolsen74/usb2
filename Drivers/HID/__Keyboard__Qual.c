
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
	U16		IEQual;
	U16		IECode;
};

SEC_RWDATA static struct hhh ooo[] =
{
{ IEQUALIFIER_CONTROL,		0x63 },		// 000700E0 LControl
{ IEQUALIFIER_LSHIFT,		0x60 },		// 000700E1 LShift
{ IEQUALIFIER_LALT,			0x64 },		// 000700E2 LAlt
{ IEQUALIFIER_LCOMMAND,		0x66 },		// 000700E3 LAmiga
{ IEQUALIFIER_CONTROL,		0x63 },		// 000700E4 Right Control
{ IEQUALIFIER_RSHIFT,		0x61 },		// 000700E5 Right Shift
{ IEQUALIFIER_RALT,			0x65 },		// 000700E6 Right Alt
{ IEQUALIFIER_RCOMMAND,		0x67 },		// 000700E7 Right Amiga
{ 0, 0 }
};

SEC_CODE void __Keyboard_Qual( 
	struct USBBase *usbbase,
	struct HID_ReportNode *rn,
	struct RealRequest *ioreq,
	struct HID_BitNode *bn,
	U32 idx,
	U32 id )
{
U32 code;
S32 val;
U32 cnt;

	for( cnt=0 ; cnt<8 ; cnt++ )
	{
		if ( id == 0x000700e0+cnt )
		{
			val = ( Report_Read_Value( usbbase, ioreq, bn, idx, FALSE )) ? TRUE : FALSE ;

			if ( rn->Controls[cnt] != val )
			{
				rn->Controls[cnt] = val;

				if ( val )
				{
					usbbase->usb_HID_Qualifiers |= ooo[cnt].IEQual;
					code = ooo[cnt].IECode;
				}
				else
				{
					usbbase->usb_HID_Qualifiers &= ~ooo[cnt].IEQual;
					code = ooo[cnt].IECode | IECODE_UP_PREFIX;
				}

				Input_Raw_Key( usbbase, rn, code );

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
			}
			break;
		}
	}
}

// --
