
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
SiS163U_SubmitRXCheck( struct EthernetUnit *unit )
{
	struct USB2_EPResource *resource;
	struct USB2_IORequest *usbio;

	resource = unit->unit_Bulk_Rx_Resource;
	usbio = resource->IORequests[0];
	usbio->io_Command = CMD_READ;
	usbio->io_Data = resource->Buffers[0];
	// A receive record can span multiple USB packets and ends with a status trailer.
	usbio->io_Length = resource->BufferSize;
	unit->unit_RxActive = TRUE;

	SendIO( (PTR) usbio );
}

// --
