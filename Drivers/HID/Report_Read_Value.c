
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

/*
** Called from Subtasks
*/

SEC_CODE S32 Report_Read_Value( struct USBBase *usbbase, struct RealRequest *ioreq, struct HID_BitNode *bn, U32 index, U32 Signed )
{
S32 offset;
U32 width;
U32 val;
U32 pos;
U32 cnt;
U8 *data;
U8 mask;
U8 chr;

	data	= ioreq->req_Public.io_Data;
	width	= bn->BitSize;
	offset	= bn->BitOffset + ( width * index );
	mask	= 0;
	chr		= 0;
	val		= 0;

	for( cnt=0 ; cnt < width; cnt++ )
	{
		pos		= offset + cnt;
		chr		= data[ pos / 8 ];
		mask	= ( 1U << ( pos & 7 ));

		if ( chr & mask )
		{
			val |= ( 1U << cnt );
		}
	}

	if (( Signed ) && ( chr & mask ))
	{
		for( ; cnt < 32 ; cnt++ )
		{
			val |= ( 1u << cnt );
		}
	}

// usbbase->usb_IExec->DebugPrintF( "Val: $%08lx, v: %ld\n", val, val );

	return( val );
}

// --
