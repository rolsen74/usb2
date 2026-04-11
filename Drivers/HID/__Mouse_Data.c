
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

SEC_CODE static void __Mouse_Data_Array(
	struct USBBase *usbbase UNUSED, 
	struct HID_ReportNode *rn UNUSED,
	struct RealRequest *ioreq UNUSED,
	struct HID_BitNode *bn )
{
struct HID_UsageNode *un;

	un = bn->UsageHeader.uh_Head;

	while( un )
	{
		switch( un->Min )
		{
			// add usage to handle here

			default:
			{
//				usbbase->usb_IExec->DebugPrintF( "Unsupported Mouse Array Usage : $%08lx\n", un->Min );
				break;
			}
		}

		un = Node_Next( un );
	}
}

// --

SEC_CODE static void __Mouse_Data_Variable(
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
				case 0x00010030:	// Mouse Move X
				case 0x00010031:	// Mouse Move Y
				{
					__Mouse_Move( usbbase, rn, ioreq, bn, idx, id );
					break;
				}

				case 0x00010038:	// Mouse Scroll Wheel
				{
					__Mouse_Scroll( usbbase, rn, ioreq, bn, idx, id );
					break;
				}

				case 0x00090001:	// Left Mouse Button
				case 0x00090002:	// Middle Mouse Button
				case 0x00090003:	// Right Mouse Button
				case 0x00090004:	// 4th Mouse Button
				case 0x00090005:	// 5th Mouse Button
				{
					__Mouse_Button( usbbase, rn, ioreq, bn, idx, id );
					break;
				}

				default:
				{
//					usbbase->usb_IExec->DebugPrintF( "Unsupported Mouse Variable Usage : $%08lx\n", id );
					break;
				}
			}
		}

		un = Node_Next( un );
	}
}

// --

SEC_CODE void __Mouse_Data( struct USBBase *usbbase, struct HID_ReportNode *rn, struct RealRequest *ioreq )
{
struct HID_BitNode *bn;

	bn = rn->BitHeader.uh_Head;

	while( bn )
	{
		if ( bn->Flags & 2 )
		{
			// Variable
			__Mouse_Data_Variable( usbbase, rn, ioreq, bn );
		}
		else
		{
			// Array
			__Mouse_Data_Array( usbbase, rn, ioreq, bn );
		}

		bn = Node_Next( bn );
	}
}

// --
