
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
Tablet_Data( struct USBBase *usbbase, struct HID_ReportNode *rn, struct RealRequest *ioreq )
{
	struct HID_BitNode *bn;

	U8 *data;
	U32 id;
	U32 index;
	U32 usage;
	U32 range;
	U32 buttons;
	U32 bit;
	S32 value;

	data = ioreq->req_Public.io_Data;
	id = ( rn->HIDData->NumberedInput ) ? data[0] : 0;

	for ( bn = Header_Head( &rn->BitHeader ); bn; bn = Node_Next( bn ) )
	{
		if ( ( bn->ReportID != id ) || ( ( bn->Flags & 7 ) != 2 ) )
		{
			continue;
		}

		for ( index = 0; index < bn->BitCount; index++ )
		{
			usage = Report_Get_Usage( bn, index, TRUE );

			switch ( usage )
			{
				case 0x00010030:
					rn->Pointer_X = Report_Read_Axis( usbbase, ioreq, bn, index, &rn->Pointer_RangeX );
					break;

				case 0x00010031:
					rn->Pointer_Y = Report_Read_Axis( usbbase, ioreq, bn, index, &rn->Pointer_RangeY );
					break;

				case 0x000d0030: // Tip pressure
					rn->Tablet_Pressure = Report_Read_Axis( usbbase, ioreq, bn, index, &range );
					rn->Tablet_HasPressure = ( range > 1 );

					if ( rn->Tablet_HasPressure )
					{
						rn->Tablet_Pressure = ( (U64) rn->Tablet_Pressure * 0x7fffffffU ) / ( range - 1 );
					}
					break;

				case 0x000d0032: // In range
					value = Report_Read_Value( usbbase, ioreq, bn, index, FALSE );
					rn->Tablet_InRange = ( value != 0 );
					rn->Tablet_HasProximity = TRUE;

					if ( ! rn->Tablet_InRange )
					{
						rn->Tablet_Switches = 0;
						rn->Tablet_Pressure = 0;
					}
					break;

				case 0x000d0042: // Tip switch
				case 0x000d0044: // Barrel switch
				case 0x000d005a: // Secondary barrel switch
					bit = ( usage == 0x000d0042 ) ? 1 : ( ( usage == 0x000d0044 ) ? 2 : 4 );
					value = Report_Read_Value( usbbase, ioreq, bn, index, FALSE );

					if ( value )
					{
						rn->Tablet_Switches |= bit;
					}
					else
					{
						rn->Tablet_Switches &= ~bit;
					}
					break;
			}
		}
	}

	if ( ! rn->Tablet_HasProximity )
	{
		rn->Tablet_InRange = TRUE;
	}

	// Preserve the last position until both axis reports have arrived.
	if ( ( ! rn->Pointer_RangeX ) || ( ! rn->Pointer_RangeY ) )
	{
		return;
	}

	buttons = ( rn->Tablet_InRange ) ? rn->Tablet_Switches : 0;

	Input_Pointer_Position( usbbase, rn, TRUE, buttons );

	for ( index = 0; index < 3; index++ )
	{
		HID_Mouse_Button( usbbase, rn, index, buttons & ( 1U << index ) );
	}
}

// --
