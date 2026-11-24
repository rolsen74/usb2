
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Radio.h"

// --

void
RTL8188EU_StopRX( struct EthernetUnit *unit )
{
	struct IORequest *usbio;

	if ( ! unit->unit_RxActive )
	{
		return;
	}

	usbio = (PTR) unit->unit_Bulk_Rx_Resource->IORequests[0];

	AbortIO( usbio );

	WaitIO( usbio );

	unit->unit_RxActive = FALSE;
}

// --
