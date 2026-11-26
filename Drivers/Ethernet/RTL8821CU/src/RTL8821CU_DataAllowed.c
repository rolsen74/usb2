
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

S32
RTL8821CU_DataAllowed( const struct EthernetUnit *unit, const struct IOSana2Req *ioreq )
{
	return ( ( unit->unit_RadioReady ) && ( unit->unit_TxReady ) && ( ! unit->unit_Detached ) &&
		( ( ( unit->unit_Online ) && ( ! unit->unit_ManualOffline ) ) || ( ( unit->unit_AssocID ) &&
			( ( ! unit->unit_ManualOffline ) || ( unit->unit_Handshake ) ) &&
			( unit->unit_Encryption == S2ENC_CCMP ) &&
			( ioreq->ios2_PacketType == RTL8821CU_EAPOL ) &&
			( ! ( ioreq->ios2_Req.io_Flags & SANA2IOF_RAW ) ) &&
			( ( ioreq->ios2_Req.io_Command == CMD_READ ) || ( ioreq->ios2_Req.io_Command == CMD_WRITE ) ) ) ) );
}

// --
