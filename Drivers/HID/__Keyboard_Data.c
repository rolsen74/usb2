
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

SEC_CODE static void __Keyboard_Data_Array( 
	struct USBBase *usbbase, 
	struct HID_ReportNode *rn,
	struct RealRequest *ioreq,
	struct HID_BitNode *bn )
{
struct HID_UsageNode *un;
U32 idx;
U32 id;

	/*
	** This code and its subfunction are weak
	** too much is hard coded (like 6 char buffers)
	** But currently it works for me
	*/

	un = bn->UsageHeader.uh_Head;

	idx = 0;	// Index is pr. BitNode

	while( un )
	{
		id = un->Min;	// Unused for now

		if ( ! (( un->Max < 0x00070000 ) || ( un->Min > 0x000700ff )))
		{
			__Keyboard_Keys( usbbase, rn, ioreq, bn, idx, id );
		}
//		else
//		{
//			usbbase->usb_IExec->DebugPrintF( "Unsupported Keyboard Array Usage : $%08lx\n", un->Min );
//		}

		un = Node_Next( un );
	}
}

// --

SEC_CODE static void __Keyboard_Data_Variable(
	struct USBBase *usbbase, 
	struct HID_ReportNode *rn,
	struct RealRequest *ioreq,
	struct HID_BitNode *bn )
{
struct HID_UsageNode *un;
U32 idx;
U32 id;

	un = bn->UsageHeader.uh_Head;

	idx = 0;	// Index is pr. BitNode

	while( un )
	{
		for( id = un->Min ; id <= un->Max ; idx++, id++ )
		{
			switch( id )
			{
				case 0x000700E0: // Left Control
				case 0x000700E1: // Left Shift
				case 0x000700E2: // Left Alt
				case 0x000700E3: // Left Amiga
				case 0x000700E4: // Right Control
				case 0x000700E5: // Right Shift
				case 0x000700E6: // Right Alt
				case 0x000700E7: // Right Amiga
				{
					__Keyboard_Qual( usbbase, rn, ioreq, bn, idx, id );
					break;
				}

				default:
				{
//					usbbase->usb_IExec->DebugPrintF( "Unsupported Keyboard Variable Usage : $%08lx\n", id );
					break;
				}
			}
		}

		un = Node_Next( un );
	}
}

// --

SEC_CODE void __Keyboard_Data( 
	struct USBBase *usbbase, 
	struct HID_ReportNode *rn, 
	struct RealRequest *ioreq )
{
struct HID_BitNode *bn;

	bn	= rn->BitHeader.uh_Head;

	while( bn )
	{
		if ( bn->Flags & 2 )
		{
			// Variable
			__Keyboard_Data_Variable( usbbase, rn, ioreq, bn );
		}
		else
		{
			// Array
			__Keyboard_Data_Array( usbbase, rn, ioreq, bn );
		}

		bn = Node_Next( bn );
	}
}

// --
