
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "HID.h"
#include <intuition/intuition.h>

// --

SEC_CODE static U32
Tablet_Axis ( struct HID_BitNode * bn, S64 value, U32 * range )
{
	S64 span;

	span = bn->MaxLogical - bn->MinLogical;

	if ( ( span <= 0 ) || ( span >= 0xffffffffLL ) )
	{
		*range = 0;

		return ( 0 );
	}

	value = MAX( bn->MinLogical, MIN( bn->MaxLogical, value ) );
	*range = (U32) span + 1;

	return ( (U32) ( value - bn->MinLogical ) );
}

// --

SEC_CODE void
Tablet_Data ( struct USBBase * usbbase, struct HID_ReportNode * rn, struct RealRequest * ioreq )
{
	struct IENewTablet tablet;
	struct InputEvent * event;
	struct IOStdReq * input;
	struct HID_BitNode * bn;
	struct HID_UsageNode * un;
	struct TagItem tags[4];

	S32 raw;
	S64 value;
	U32 index;
	U32 usage;
	U32 range;
	U32 pressure;
	U32 count;
	U8 buttons;
	S32 proximity;
	S32 has_pressure;
	S32 has_proximity;

	buttons = 0;
	pressure = 0;
	count = 0;
	proximity = TRUE;
	has_pressure = FALSE;
	has_proximity = FALSE;

	if ( ( ! ioreq->req_Public.io_Data ) ||
		( (U64) MIN( ioreq->req_Public.io_Actual, ioreq->req_Public.io_Length ) * 8 < rn->Offset_In ) )
	{
		return;
	}

	MEM_SET( &tablet, 0, sizeof( tablet ) );

	for ( bn = Header_Head( &rn->BitHeader ); bn; bn = Node_Next( bn ) )
	{
		// Data, Variable, Absolute input fields only.
		if ( ( ( bn->Flags & 7 ) != 2 ) || ( ! bn->BitSize ) || ( bn->BitSize > 32 ) )
		{
			continue;
		}

		index = 0;

		for ( un = Header_Head( &bn->UsageHeader ); un; un = Node_Next( un ) )
		{
			for ( usage = un->Min; index < bn->BitCount; index++, usage++ )
			{
				raw = Report_Read_Value( usbbase, ioreq, bn, index, bn->MinLogical < 0 );

				value = ( bn->MinLogical < 0 ) ? (S64) raw : (S64) (U32) raw;

				switch ( usage )
				{
					case 0x00010030: // X
						tablet.ient_TabletX = Tablet_Axis( bn, value, &range );

						tablet.ient_RangeX = range;
						break;

					case 0x00010031: // Y
						tablet.ient_TabletY = Tablet_Axis( bn, value, &range );

						tablet.ient_RangeY = range;
						break;

					case 0x000d0030: // Tip pressure
						pressure = Tablet_Axis( bn, value, &range );

						if ( range > 1 )
						{
							pressure = ( (U64) pressure * 0x7fffffffU ) / ( range - 1 );
							has_pressure = TRUE;
						}
						break;

					case 0x000d0032: // In Range
						proximity = ( value != 0 );
						has_proximity = TRUE;
						break;

					case 0x000d0042: // Tip switch
						buttons |= value ? 1 : 0;
						break;

					case 0x000d0044: // Barrel switch
						buttons |= value ? 2 : 0;
						break;

					case 0x000d005a: // Secondary barrel switch
						buttons |= value ? 4 : 0;
						break;
				}

				if ( usage == un->Max )
				{
					index++;
					break;
				}
			}
		}
	}

	if ( ( ! tablet.ient_RangeX ) || ( ! tablet.ient_RangeY ) )
	{
		return;
	}

	if ( ! proximity )
	{
		buttons = 0;
		pressure = 0;
	}

	if ( has_pressure )
	{
		tags[count].ti_Tag = TABLETA_Pressure;
		tags[count++].ti_Data = pressure;
	}

	if ( has_proximity )
	{
		tags[count].ti_Tag = TABLETA_InProximity;
		tags[count++].ti_Data = proximity;
	}

	tags[count].ti_Tag = TABLETA_ButtonBits;
	tags[count++].ti_Data = buttons;
	tags[count].ti_Tag = TAG_DONE;
	tags[count].ti_Data = 0;
	tablet.ient_TagList = tags;

	event = &rn->Input_Event;
	event->ie_NextEvent = NULL;
	event->ie_Class = IECLASS_NEWPOINTERPOS;
	event->ie_SubClass = IESUBCLASS_NEWTABLET;
	event->ie_Code = IECODE_NOBUTTON;
	event->ie_Qualifier = usbbase->usb_HID_Qualifiers;
	event->ie_EventAddress = &tablet;

	input = &rn->Input_IOReq;
	input->io_Command = IND_WRITEEVENT;
	input->io_Data = event;
	input->io_Length = sizeof( *event );

	IO_DO( input );

	Tablet_Buttons( usbbase, rn, buttons );
}

// --
