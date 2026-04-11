
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

void Node_Report_Free( struct USBBase *usbbase, struct HID_ReportNode *node )
{
PTR n;

	if ( ! node )
	{
		goto bailout;
	}

	while( TRUE )
	{
		n = NODE_REMHEAD( & node->UsageHeader );

		if ( ! n )
		{
			break;
		}

		Node_Usage_Free( usbbase, n );
	}

	while( TRUE )
	{
		n = NODE_REMHEAD( & node->BitHeader );

		if ( ! n )
		{
			break;
		}

		Node_Bit_Free( usbbase, n );
	}

	// Stop Driver in here? if running

	MEM_FREEVEC( node );

bailout:

	return;
}

// --
