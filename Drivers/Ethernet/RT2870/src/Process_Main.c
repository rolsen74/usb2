
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
Process_Main( struct EthernetUnit *unit )
{
	struct USB2_NotifyMessage *msg;
	struct USB2_Register *reg;

	U32 mask;
	U32 receiveBit;
	S32 detached;

	reg = unit->unit_USB_Register;
	detached = FALSE;

	do
	{
		receiveBit = unit->unit_BeaconResource ? unit->unit_BeaconResource->MsgPortBit : 0;

		mask = Wait( reg->Stack_MsgPortBit | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_E | receiveBit );

		while(( msg = (PTR) GetMsg( reg->Stack_MsgPort ) ))
		{
			if ( msg->nm_Command == USBNC_Detached )
			{
				detached = TRUE;

				ObtainSemaphore( & unit->unit_CommandLock );

				unit->unit_Detached = TRUE;

				Sana2_SignalEvent( & unit->unit_Pending, S2EVENT_ERROR | S2EVENT_HARDWARE, FALSE );

				Sana2_AbortPending( & unit->unit_Pending, NULL, NULL, IOERR_ABORTED, 0 );

				ReleaseSemaphore( & unit->unit_CommandLock );
			}

			ReplyMsg( (PTR) msg );
		}

		if ( ( ! detached ) && ( ! ( mask & SIGBREAKF_CTRL_C ) ) && ( mask & ( SIGBREAKF_CTRL_E | receiveBit ) ) )
		{
			Process_RadioCommands( unit );
		}
	}
	while( ( ! detached ) && ( ! ( mask & SIGBREAKF_CTRL_C ) ) );
}

// --
