
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

SEC_CODE static S32
ReadKeys( struct USBBase *usbbase, struct RealRequest *ioreq, struct HID_BitNode *bn, U8 *keys )
{
	U32 index;
	U32 usage;
	S32 raw;
	S64 value;

	for ( index = 0; index < bn->BitCount; index++ )
	{
		raw = Report_Read_Value( usbbase, ioreq, bn, index, bn->MinLogical < 0 );
		value = ( bn->MinLogical < 0 ) ? (S64) raw : (S64) (U32) raw;

		if ( ( value < bn->MinLogical ) || ( value > bn->MaxLogical ) )
		{
			continue;
		}

		if ( bn->Flags & 2 )
		{
			usage = Report_Get_Usage( bn, index, TRUE );

			if ( ! value )
			{
				continue;
			}
		}
		else
		{
			usage = Report_Get_Usage( bn, value - bn->MinLogical, FALSE );
		}

		if ( ( usage >= 0x00070001 ) && ( usage <= 0x00070003 ) )
		{
			return ( FALSE );
		}

		if ( ( usage >= 0x00070004 ) && ( usage <= 0x000700ff ) )
		{
			usage &= 255;
			keys[usage / 8] |= 1U << ( usage & 7 );
		}
	}

	return ( TRUE );
}

// --

SEC_CODE void
__Keyboard_Data( struct USBBase *usbbase, struct HID_ReportNode *rn, struct RealRequest *ioreq )
{
	struct HID_ReportLayout *layout;
	struct HID_BitNode *bn;
	struct HIDData *hd;

	U8 keys[32];
	U8 *data;
	U32 id;
	U32 report;
	U32 index;

	hd = rn->HIDData;
	data = ioreq->req_Public.io_Data;
	id = ( hd->NumberedInput ) ? data[0] : 0;

	MEM_SET( keys, 0, sizeof( keys ) );

	for ( bn = Header_Head( &rn->BitHeader ); bn; bn = Node_Next( bn ) )
	{
		if ( ( bn->ReportID == id ) && ( ! ReadKeys( usbbase, ioreq, bn, keys ) ) )
		{
			return;
		}
	}

	MEM_COPY( keys, hd->Layouts[id]->Keys, sizeof( keys ) );

	for ( report = 0; report < 256; report++ )
	{
		layout = hd->Layouts[report];

		if ( ( layout ) && ( layout->Owner[HID_Report_Input] == rn ) )
		{
			for ( index = 0; index < sizeof( keys ); index++ )
			{
				keys[index] |= layout->Keys[index];
			}
		}
	}

	__Keyboard_Keys( usbbase, rn, keys );
}

// --
