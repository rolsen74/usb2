
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
Handle_BulkInMessages( struct devUnit *unit )
{
	struct USB2IFace *IUSB2;
	struct USB2_IORequest *usbio;
	struct IONode *node;

	IUSB2 = unit->unit_IUSB2;

	while(( usbio = (PTR) IExec->GetMsg( unit->unit_USBBulkIn->MsgPort ) ))
	{
		node = usbio->io_UserData;

		IExec->Remove( & node->Node );

		node->Active = FALSE;

#ifdef WIRED_TRACE_BULK
		if ( ! node->AbortError )
		{
			WIRED_TRACE_BULK( unit, usbio, TRUE );
		}
#endif

		if ( usbio->io_Error == USB2Err_Device_Detached )
		{
			unit->unit_Running = FALSE;

			SignalSanaEvent( unit, S2EVENT_ERROR | S2EVENT_RX | S2EVENT_HARDWARE );
		}
		else if ( ( ! node->AbortError ) && ( unit->unit_Online ) && ( unit->unit_LinkUp ) &&
			( usbio->io_Error == USB2Err_NoError ) && ( usbio->io_Actual <= node->BufferSize ) )
		{
			if ( ! Wired_DecodeRX( unit, node->Buffer, usbio->io_Actual ) )
			{
				unit->unit_BadData++;

				SignalSanaEvent( unit, S2EVENT_ERROR | S2EVENT_RX | S2EVENT_HARDWARE );
			}
		}
		else
		{
			WIRED_RESET_RX( unit );

			if ( ( ! node->AbortError ) && ( unit->unit_Online ) )
			{
				SignalSanaEvent( unit, S2EVENT_ERROR | S2EVENT_RX | S2EVENT_HARDWARE );

				if ( ( usbio->io_Error == USB2Err_Host_Stall ) && ( ! unit->unit_RxHalted ) )
				{
					unit->unit_RxHalted = ! IUSB2->USB2_EPRes_Destall( unit->unit_USBBulkIn );
				}
			}
		}

		node->AbortError = 0;

		IExec->AddTail( & unit->unit_USBBulkInFreeList, & node->Node );
	}

	Schedule_RxIOReq( unit );
}

// --
