
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

SEC_CODE void __Mouse_Scroll( 
	struct USBBase *usbbase,
	struct HID_ReportNode *rn UNUSED,
	struct RealRequest *ioreq,
	struct HID_BitNode *bn,
	U32 idx,
	U32 id UNUSED )
{
S32 val;

	val = Report_Read_Value( usbbase, ioreq, bn, idx, TRUE );

	Input_Mouse_Scroll( usbbase, rn, val );
}

// --
