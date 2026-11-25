
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define WIRED_HALT_RETRIES 3U

// --

static void
RecoverEndpoints( struct devUnit *unit )
{
	struct USB2IFace *usb2;

	usb2 = unit->unit_IUSB2;

	if ( ( unit->unit_RxHalted ) && ( unit->unit_RxRecovery < WIRED_HALT_RETRIES ) )
	{
		unit->unit_RxRecovery++;

		if ( usb2->USB2_EPRes_Destall( unit->unit_USBBulkIn ) )
		{
			unit->unit_RxHalted = FALSE;
			unit->unit_RxRecovery = 0;

			Schedule_RxIOReq( unit );
		}
	}

	if ( ( unit->unit_TxHalted ) && ( unit->unit_TxRecovery < WIRED_HALT_RETRIES ) )
	{
		unit->unit_TxRecovery++;

		if ( usb2->USB2_EPRes_Destall( unit->unit_USBBulkOut ) )
		{
			unit->unit_TxHalted = FALSE;
			unit->unit_TxRecovery = 0;

			Schedule_TxIOReq( unit );
		}
	}
}

// --

void
Wired_UpdateLink( struct devUnit *unit )
{
	S32 connected;

	if ( unit->unit_ControlNeedsSetup )
	{
		return;
	}

	connected = Wired_PHYLink( unit );

	if ( ( connected >= 0 ) && ( connected != unit->unit_LinkUp ) )
	{
		unit->unit_LinkUp = connected;

		if ( connected )
		{
			SignalSanaEvent( unit, S2EVENT_CONNECT );

			Schedule_RxIOReq( unit );

			Schedule_TxIOReq( unit );
		}
		else
		{
			AbortPacketIO( unit, S2WERR_UNIT_DISCONNECTED );

			Sana2_AbortPendingWithReply( & unit->unit_TxList, NULL, NULL,
				S2ERR_OUTOFSERVICE, S2WERR_UNIT_DISCONNECTED, ReplySanaRequest, unit );

			SignalSanaEvent( unit, S2EVENT_DISCONNECT );
		}
	}

	if ( unit->unit_Online )
	{
		RecoverEndpoints( unit );
	}
}

// --
