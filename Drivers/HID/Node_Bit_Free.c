
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

void Node_Bit_Free( struct USBBase *usbbase, struct HID_BitNode *node )
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

	MEM_FREEVEC( node );

bailout:

	return;
}

// --
