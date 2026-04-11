
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

void Node_Usage_Free( struct USBBase *usbbase, struct HID_UsageNode *node )
{
	if ( ! node )
	{
		goto bailout;
	}


	MEM_FREEVEC( node );

bailout:

	return;
}

// --
