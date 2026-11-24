
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

SEC_CODE U32
Report_Read_Axis( struct USBBase *usbbase, struct RealRequest *ioreq, struct HID_BitNode *bn, U32 index, U32 *range )
{
	S64 span;
	S64 value;
	S32 raw;

	span = bn->MaxLogical - bn->MinLogical;

	if ( ( span <= 0 ) || ( span >= 0xffffffffLL ) )
	{
		*range = 0;

		return ( 0 );
	}

	raw = Report_Read_Value( usbbase, ioreq, bn, index, bn->MinLogical < 0 );
	value = ( bn->MinLogical < 0 ) ? (S64) raw : (S64) (U32) raw;
	value = MAX( bn->MinLogical, MIN( bn->MaxLogical, value ) );
	*range = (U32) span + 1;

	return ( (U32) ( value - bn->MinLogical ) );
}

// --
