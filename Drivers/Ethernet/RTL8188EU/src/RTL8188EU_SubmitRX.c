
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

	if ( ( ! unit->unit_RadioReady ) || ( unit->unit_Detached ) )
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

	if ( ( ! request ) && ( ! unit->unit_Online ) && ( ! ( RTL8188EU_DeferredLink( unit ) && unit->unit_AssocID ) ) )
	{
		return;
	}

	Wireless_SubmitRX( unit->unit_Bulk_Rx_Resource, &unit->unit_RxActive );
}

// --
