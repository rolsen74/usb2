
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <usb2wifi.h>
#include "All.h"
#include "Radio.h"
#include "WiFiCommand.h"

// --

void
Process_Main( struct EthernetUnit *unit )
{
	struct USB2_NotifyMessage *msg;

	U32 wait;
	U32 mask;

	wait = unit->unit_USB_Register->Stack_MsgPortBit | unit->unit_Bulk_Rx_Resource->MsgPortBit | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_E;

	Wireless_StartAuto( USB2WIFI_RTL8188, unit->unit_UnitNr );

	while( ! unit->unit_Detached )
	{
		mask = Wait( wait );

		if ( mask & SIGBREAKF_CTRL_C )
		{
			return;
		}

		if ( mask & unit->unit_USB_Register->Stack_MsgPortBit )
		{
			while(( msg = (PTR) GetMsg( unit->unit_USB_Register->Stack_MsgPort ) ))
			{
				if ( msg->nm_Command == USBNC_Detached )
				{
					ObtainSemaphore( & unit->unit_CommandLock );

					unit->unit_Detached = TRUE;

					RTL8188EU_DropLink( unit );

					Sana2_SignalEvent( & unit->unit_Pending, S2EVENT_ERROR | S2EVENT_HARDWARE );

					Sana2_AbortPending( & unit->unit_Pending, NULL, NULL, IOERR_ABORTED, 0 );

					ReleaseSemaphore( & unit->unit_CommandLock );
				}

				ReplyMsg( (PTR) msg );
			}
		}

		if ( ( ! unit->unit_Detached ) && ( mask & SIGBREAKF_CTRL_E ) )
		{
			Process_RadioCommands( unit );
		}

		if ( ( ! unit->unit_Detached ) && ( mask & unit->unit_Bulk_Rx_Resource->MsgPortBit ) )
		{
			RTL8188EU_CompleteRX( unit );
		}
	}
}

// --
