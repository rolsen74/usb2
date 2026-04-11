
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
{ IEQUALIFIER_LEFTBUTTON,	IECODE_LBUTTON },
{ IEQUALIFIER_RBUTTON,		IECODE_RBUTTON },
{ IEQUALIFIER_MIDBUTTON,	IECODE_MBUTTON },
{ 0,						IECODE_4TH_BUTTON },
{ 0,						IECODE_5TH_BUTTON },
{ 0, 0 }
};

SEC_CODE void __Mouse_Button( 
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

	for( cnt=0 ; cnt<5 ; cnt++ )
	{
		if ( id == 0x00090001+cnt )
		{
			val = ( Report_Read_Value( usbbase, ioreq, bn, idx, FALSE )) ? TRUE : FALSE ;

// usbbase->usb_IExec->DebugPrintF( "$%08lx : val %ld\n", id, val );

			if ( rn->Buttons[cnt] != val )
			{
				rn->Buttons[cnt] = val;

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

				Input_Mouse_Button( usbbase, rn, code );
			}

			break;
		}
	}
}

// --
