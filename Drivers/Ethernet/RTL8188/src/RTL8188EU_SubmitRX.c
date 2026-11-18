
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
RTL8188EU_SubmitRX( struct EthernetUnit *unit )
{
	struct IOSana2Req *request;
	struct USB2_EPResource *resource;
	struct USB2_IORequest *usbio;

	if ( ( ! unit->unit_RadioReady ) || ( unit->unit_RxActive ) || ( unit->unit_Detached ) )
	{
		return;
	}

	request = (PTR) GetHead( & unit->unit_Pending );

	while( request )
	{
		if ( request->ios2_Req.io_Command == S2_READMGMT )
		{
			break;
		}

		request = (PTR) GetSucc( (PTR) request );
	}

	if ( ( ! request ) && ( ! unit->unit_Online ) )
	{
		return;
	}

	resource = unit->unit_Bulk_Rx_Resource;
	usbio = resource->IORequests[0];
	usbio->io_Command = CMD_READ;
	usbio->io_Data = resource->Buffers[0];
	usbio->io_Length = resource->BufferSize;

	unit->unit_RxActive = TRUE;

	SendIO( (PTR) usbio );
}

// --
