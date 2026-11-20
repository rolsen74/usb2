
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
	U32 rxMask;
	S32 detached;

	reg = unit->unit_USB_Register;
	rxMask = unit->unit_Bulk_Rx_Resource->MsgPortBit;
	detached = FALSE;

	if ( ( unit->unit_RadioInitialized ) && ( unit->unit_StationAddressProgrammed ) )
	{
		SiS163U_StartRXCheck( unit );
	}

	do
	{
		mask = Wait( reg->Stack_MsgPortBit | rxMask | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_E );

		while(( msg = (PTR) GetMsg( reg->Stack_MsgPort ) ))
		{
			if ( msg->nm_Command == USBNC_Detached )
			{
				detached = TRUE;

				ObtainSemaphore( & unit->unit_CommandLock );

				unit->unit_Detached = TRUE;

				SignalSana2Event( unit, S2EVENT_ERROR | S2EVENT_HARDWARE );

				AbortPendingCommands( unit, NULL, NULL );

				ReleaseSemaphore( & unit->unit_CommandLock );
			}

			ReplyMsg( (PTR) msg );
		}

		if ( ( ! detached ) && ( ! ( mask & SIGBREAKF_CTRL_C ) ) )
		{
			if ( mask & SIGBREAKF_CTRL_E )
			{
				Process_RadioCommands( unit );
			}

			if ( mask & rxMask )
			{
				SiS163U_CompleteRXCheck( unit );
			}
		}
	}
	while( ( ! detached ) && ( ! ( mask & SIGBREAKF_CTRL_C ) ) );
}

// --
