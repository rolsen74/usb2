
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Commands.h"
#include "WiFiCommand.h"
#include <usb2wifi.h>
#include "Radio.h"

// --

void
Process_RadioCommands( struct EthernetUnit *unit )
{
	struct IOSana2Req *ioreq;

	S32 startWiFi;

	ObtainSemaphore( & unit->unit_CommandLock );

	startWiFi = unit->unit_WiFiStartPending && ! unit->unit_Detached;
	unit->unit_WiFiStartPending = FALSE;

	ReleaseSemaphore( & unit->unit_CommandLock );

	if ( startWiFi )
	{
		Wireless_StartAuto( USB2WIFI_RTL8188, unit->unit_UnitNr );
	}

	ObtainSemaphore( & unit->unit_CommandLock );

	for( ;; )
	{
		ioreq = (PTR) GetHead( & unit->unit_Pending );

		while( ioreq )
		{
			if ( ( ioreq->ios2_Req.io_Command == S2_SETOPTIONS ) ||
				( ioreq->ios2_Req.io_Command == USB2WIRELESS_CMD_PROMPT ) ||
				( ioreq->ios2_Req.io_Command == S2_SETKEY ) ||
				( ioreq->ios2_Req.io_Command == S2_WRITEMGMT ) ||
				( ioreq->ios2_Req.io_Command == CMD_WRITE ) ||
				( ioreq->ios2_Req.io_Command == S2_MULTICAST ) ||
				( ioreq->ios2_Req.io_Command == S2_BROADCAST ) )
			{
				break;
			}

			ioreq = (PTR) GetSucc( (PTR) ioreq );
		}

		if ( ! ioreq )
		{
			break;
		}

		Remove( (PTR) ioreq );

		// The USB receive port belongs to this task, including during WaitIO.
		if ( ioreq->ios2_Req.io_Command == S2_SETOPTIONS )
		{
			Cmd_SetOptions( unit, ioreq );

			USBINFO( "RTL8188EU: S2_SETOPTIONS error %ld, wire error %lu, channel %lu, AID %lu, TX %lu, online %lu",
				(S32) ioreq->ios2_Req.io_Error, ioreq->ios2_WireError, (U32) unit->unit_Channel,
				(U32) unit->unit_AssocID, (U32) unit->unit_TxReady, (U32) unit->unit_Online );
		}
		else if ( ioreq->ios2_Req.io_Command == USB2WIRELESS_CMD_PROMPT )
		{
			Wireless_StartPassword( ioreq, USB2WIFI_RTL8188, unit->unit_UnitNr );
		}
		else if ( ioreq->ios2_Req.io_Command == S2_SETKEY )
		{
			Cmd_SetKey( unit, ioreq );
		}
		else
		{
			RTL8188EU_Transmit( unit, ioreq );
		}

		ReplyMsg( (PTR) ioreq );
	}

	ReleaseSemaphore( & unit->unit_CommandLock );
}

// --
