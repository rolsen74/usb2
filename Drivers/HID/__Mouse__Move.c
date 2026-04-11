
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

SEC_CODE void __Mouse_Move( 
	struct USBBase *usbbase,
	struct HID_ReportNode *rn UNUSED,
	struct RealRequest *ioreq,
	struct HID_BitNode *bn,
	U32 idx,
	U32 id )
{
S32 val;

	val = Report_Read_Value( usbbase, ioreq, bn, idx, TRUE );

	/**/ if ( id == 0x00010030 )
	{
		Input_Mouse_Move( usbbase, rn, val, 0 );
	}
	else if ( id == 0x00010031 )
	{
		Input_Mouse_Move( usbbase, rn, 0, val );
	}
}

// --
