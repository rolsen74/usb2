
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
RTL8821CU_DropLink( struct EthernetUnit *unit )
{
	U8 bssid[RTL8821CU_ADDR_SIZE];

	unit->unit_AssocID = 0;
	unit->unit_Operational = FALSE;
	unit->unit_Handshake = FALSE;
	unit->unit_EAPOLLength = 0;

	memset( unit->unit_BSSID, 0, sizeof( unit->unit_BSSID ) );

	memset( unit->unit_Keys, 0, sizeof( unit->unit_Keys ) );

	RTL8821CU_SetOnline( unit, FALSE );

	Sana2_AbortPackets( & unit->unit_Pending, S2ERR_OUTOFSERVICE, S2WERR_UNIT_OFFLINE );

	RTL8821CU_StopRX( unit );

	memset( bssid, 0, sizeof( bssid ) );

	if ( ( ! RTL8821CU_DrainTX( unit ) ) || ( ! RTL8821CU_SetAssociation( unit, bssid, 0, FALSE ) ) ||
		( ! RTL8821CU_SetTxPower( unit ) ) )
	{
		unit->unit_RadioReady = FALSE;
		unit->unit_TxReady = FALSE;

		Sana2_SignalEvent( & unit->unit_Pending, S2EVENT_ERROR | S2EVENT_HARDWARE );
	}
}

// --
