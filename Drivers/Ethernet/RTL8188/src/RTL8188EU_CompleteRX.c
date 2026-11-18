
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
RTL8188EU_CompleteRX( struct EthernetUnit *unit )
{
	struct USB2_EPResource *resource;
	struct USB2_IORequest *usbio;

	resource = unit->unit_Bulk_Rx_Resource;

	ObtainSemaphore( & unit->unit_CommandLock );

	usbio = (PTR) GetMsg( resource->MsgPort );

	if ( usbio )
	{
		unit->unit_RxActive = FALSE;

		if ( ( usbio->io_Error == USB2Err_NoError ) && ( usbio->io_Actual <= resource->BufferSize ) )
		{
			RTL8188EU_DecodeRX( unit, usbio->io_Data, usbio->io_Actual );
		}
		else if ( usbio->io_Error == USB2Err_Host_Stall )
		{
			USB2_EPRes_Destall( resource );
		}
		else
		{
			unit->unit_RadioReady = FALSE;
			unit->unit_TxReady = FALSE;

			RTL8188EU_DropLink( unit );

			SignalSana2Event( unit, S2EVENT_ERROR | S2EVENT_HARDWARE );

			AbortPendingCommands( unit, NULL, NULL );
		}

		RTL8188EU_SubmitRX( unit );
	}

	ReleaseSemaphore( & unit->unit_CommandLock );
}

// --
