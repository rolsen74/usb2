
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

struct HID_CollectionNode *Node_Collection_Alloc( struct USBBase *usbbase )
{
struct HID_CollectionNode *node;

	node = MEM_ALLOCVEC( sizeof( struct HID_CollectionNode ), TRUE );

	if ( ! node )
	{
		goto bailout;
	}


bailout:

	return( node );
}

// --
