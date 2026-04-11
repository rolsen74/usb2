
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

struct HID_ReportNode *Node_Report_Alloc( struct USBBase *usbbase, struct HIDData *hd )
{
struct HID_ReportNode *node;

	node = MEM_ALLOCVEC( sizeof( struct HID_ReportNode ), TRUE );

	if ( ! node )
	{
		goto bailout;
	}

	node->HIDData = hd;
	node->Running = TRUE;

bailout:

	return( node );
}

// --
