
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
__Mouse_Data( struct USBBase *usbbase, struct HID_ReportNode *rn, struct RealRequest *ioreq )
{
	struct HID_BitNode *bn;

	U8 *data;
	U32 id;
	U32 index;
	U32 usage;
	U32 absolute;
	U32 buttons;
	U32 bit;
	S32 dx;
	S32 dy;
	S32 value;

	data = ioreq->req_Public.io_Data;
	id = ( rn->HIDData->NumberedInput ) ? data[0] : 0;
	absolute = FALSE;
	buttons = rn->Buttons;
	dx = 0;
	dy = 0;

	for ( bn = Header_Head( &rn->BitHeader ); bn; bn = Node_Next( bn ) )
	{
		if ( ( bn->ReportID != id ) || ( ! ( bn->Flags & 2 ) ) )
		{
			continue;
		}

		for ( index = 0; index < bn->BitCount; index++ )
		{
			usage = Report_Get_Usage( bn, index, TRUE );

			switch ( usage )
			{
				case 0x00010030:
				case 0x00010031:
					if ( bn->Flags & 4 )
					{
						value = Report_Read_Value( usbbase, ioreq, bn, index, bn->MinLogical < 0 );

						if ( usage == 0x00010030 )
						{
							dx = value;
						}
						else
						{
							dy = value;
						}
					}
					else if ( usage == 0x00010030 )
					{
						rn->Pointer_X = Report_Read_Axis( usbbase, ioreq, bn, index, &rn->Pointer_RangeX );
						absolute = TRUE;
					}
					else
					{
						rn->Pointer_Y = Report_Read_Axis( usbbase, ioreq, bn, index, &rn->Pointer_RangeY );
						absolute = TRUE;
					}
					break;

				case 0x00010038:
					__Mouse_Scroll( usbbase, rn, ioreq, bn, index, usage );
					break;

				case 0x00090001:
				case 0x00090002:
				case 0x00090003:
				case 0x00090004:
				case 0x00090005:
					value = Report_Read_Value( usbbase, ioreq, bn, index, FALSE );

					bit = 1U << ( usage - 0x00090001 );
					buttons = ( value ) ? ( buttons | bit ) : ( buttons & ~bit );
					break;
			}
		}
	}

	if ( absolute )
	{
		Input_Pointer_Position( usbbase, rn, FALSE, buttons );
	}

	for ( index = 0; index < 5; index++ )
	{
		HID_Mouse_Button( usbbase, rn, index, buttons & ( 1U << index ) );
	}

	Input_Mouse_Move( usbbase, rn, dx, dy );
}

// --
