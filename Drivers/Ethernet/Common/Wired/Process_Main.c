
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
unit_Process_MainLoop( struct devUnit *unit )
{
U32 mask;
U32 link_signal;

	link_signal = 1UL << unit->unit_LinkPort->mp_SigBit;

	Wired_ArmLinkTimer( unit );

	while( unit->unit_Running == TRUE )
	{
		mask = IExec->Wait( 0
			| SIGBREAKF_CTRL_C
			| link_signal
			| unit->unit_USBRegister->Stack_MsgPortBit
			| unit->unit_BeginIOMsgPortBit
			| unit->unit_USBBulkIn->MsgPortBit
			| unit->unit_USBBulkOut->MsgPortBit

		);

		/* Handle Messages from USB Stack */
		if ( mask & unit->unit_USBRegister->Stack_MsgPortBit )
		{
			Handle_USBFunction( unit );
		}

		if ( ( ! unit->unit_Running ) || ( mask & SIGBREAKF_CTRL_C ) )
		{
			unit->unit_Running = FALSE;

			break;
		}

		if ( ( mask & link_signal ) && ( IExec->CheckIO( & unit->unit_LinkTimer->Request ) ) )
		{
			IExec->WaitIO( & unit->unit_LinkTimer->Request );

			unit->unit_LinkTimerPending = FALSE;

			if ( unit->unit_Configured )
			{
				Wired_UpdateLink( unit );
			}

			Wired_ArmLinkTimer( unit );
		}

		/* Handle Messages from Network Stack */
		if ( mask & unit->unit_BeginIOMsgPortBit )
		{
			Handle_BeginIOMessages( unit );
			Handle_AbortIOMessages( unit );
		}

		/* Handle Incomming BulkIn Packets */
		if ( mask & unit->unit_USBBulkIn->MsgPortBit )
		{
			Handle_BulkInMessages( unit );
		}

		/* Handle Outgoing BulkOut Packets */
		if ( mask & unit->unit_USBBulkOut->MsgPortBit )
		{
			Handle_BulkOutMessages( unit );
		}

	}

	if ( unit->unit_Online )
	{
		unit->unit_Online = FALSE;

		SignalSanaEvent( unit, S2EVENT_OFFLINE );
	}
}

// --
