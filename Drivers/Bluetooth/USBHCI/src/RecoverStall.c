
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

void
BTUSB_RecoverStall( struct BTUSB *usb, struct USB2_EPResource *resource, S32 error )
{
	if ( ( error != USB2Err_Host_Stall ) || ( BTUSB_StopError( usb ) ) )
	{
		return;
	}

	if ( ! usb->USB2->USB2_EPRes_Destall( resource ) )
	{
		DebugPrintF( "Bluetooth USB: could not clear a halted HCI endpoint\n" );
	}
}

// --
