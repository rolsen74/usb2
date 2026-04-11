
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

SEC_CODE S32 Report_Read_Item( struct USBBase *usbbase, struct HIDData *hd UNUSED, struct pintern *pi )
{
S32 error;

	error = TRUE;

	switch( pi->bSize )
	{
		case 0: // 0 Bytes
		{
			pi->len  = 0;
			pi->mask = 0x00000000;

			pi->sval = 0;
			pi->uval = 0;
			break;
		}

		case 1: // 1 Byte
		{
			pi->len  = 1;
			pi->mask = 0x000000ff;

			pi->sval = (S8) ( pi->data[0] << 0 );
			pi->uval = (U8) ( pi->data[0] << 0 );
			break;
		}

		case 2: // 2 Bytes
		{
			pi->len  = 2;
			pi->mask = 0x0000ffff;
			
			pi->sval = (S16) (( pi->data[1] << 8 ) | ( pi->data[0] << 0 ));
			pi->uval = (U16) (( pi->data[1] << 8 ) | ( pi->data[0] << 0 ));
			break;
		}

		case 3: // 4 Bytes
		{
			pi->len  = 4;
			pi->mask = 0xffffffff;

			pi->sval = (S32) (( pi->data[3] << 24 ) | ( pi->data[2] << 16 ) | ( pi->data[1] << 8 ) | ( pi->data[0] << 0 ));
			pi->uval = (U32) (( pi->data[3] << 24 ) | ( pi->data[2] << 16 ) | ( pi->data[1] << 8 ) | ( pi->data[0] << 0 ));
			break;
		}

		default:
		{
//			usbbase->usb_IExec->DebugPrintF( "Unknown Item Size\n" );
			goto bailout;
		}
	}

	pi->data += pi->len;

	error = FALSE;

bailout:

	return( error );
}

// --
