
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#include "Process_Main_BeginIO.c"
#include "Process_Main_Shutdown.c"
#include "Process_Main_Buffer.c"
#include "Process_Main_Direct.c"

// --

void
Process_Main( struct PrinterUnit *unit )
{
	struct USB2_NotifyMessage *msg;
	struct USB2_Register *reg;
	struct TimeRequest *timer;
	struct USB2_IORequest *usbio;

	U32 wait;
	U32 mask;
	S32 closing;
	S32 intake_drained;

	reg = unit->unit_USB_Register;
	wait = SIGBREAKF_CTRL_C | reg->Stack_MsgPortBit | unit->unit_PRT_Begin_MsgPortBit |
		unit->unit_PRT_Abort_MsgPortBit | unit->unit_Bulk_Tx_Resource->MsgPortBit |
		( 1UL << unit->unit_CloseTimerPort->mp_SigBit );

	if ( unit->unit_Dot4_Active )
	{
		wait |= unit->unit_Bulk_Rx_Resource->MsgPortBit | reg->Res_Control->MsgPortBit;
	}

	while( ! unit->unit_Detached )
	{
		mask = Wait( wait );

		if ( mask & reg->Stack_MsgPortBit )
		{
			while( ( msg = (PTR) GetMsg( reg->Stack_MsgPort ) ) )
			{
				if ( msg->nm_Command == USBNC_Detached )
				{
					unit->unit_Detached = TRUE;
				}

				ReplyMsg( (PTR) msg );
			}
		}

		if ( ( unit->unit_Detached ) || ( mask & SIGBREAKF_CTRL_C ) )
		{
			break;
		}

		if ( unit->unit_Dot4_Active )
		{
			usbio = unit->unit_Dot4_IO;

			if ( usbio )
			{
				usbio = (PTR)GetMsg( usbio->io_Message.mn_ReplyPort );

				if ( usbio )
				{
					Dot4_Complete( unit, usbio );
				}
			}
		}
		else if ( unit->unit_Buffer_Size )
		{
			__myProcess_Main_Buffer( unit );
		}
		else
		{
			__myProcess_Main_Direct( unit );
		}

		if ( unit->unit_Detached )
		{
			break;
		}

		// Completions already queued take precedence over an expired deadline.
		while ( ( timer = (PTR)GetMsg( unit->unit_CloseTimerPort ) ) )
		{
			if ( timer == unit->unit_CloseTimer.Request )
			{
				Close_Timeout( unit );
			}
			else
			{
				unit->unit_IdleTimer.Pending = FALSE;

				if ( ! unit->unit_JobError )
				{
					Buffer_Flush( unit );
				}
			}
		}

		if ( unit->unit_Detached )
		{
			break;
		}

		if ( mask & unit->unit_PRT_Abort_MsgPortBit )
		{
			Printer_Abort( unit );
		}

		// Once Closing is observed, the bridge has fenced new input.
		closing = unit->unit_PortInterface->JobClosing( unit->unit_PortUnit );

		if ( ( closing ) && ( ! unit->unit_CloseTimer.Pending ) )
		{
			Printer_TimerArm( &unit->unit_CloseTimer, 60000000 );
		}

		intake_drained = FALSE;

		if ( ( ! unit->unit_Dot4_IO ) ||
			( unit->unit_Dot4_IO != reg->Res_Control->IORequests[0] ) )
		{
			intake_drained = __myProcess_Main_BeginIO( unit );
		}

		if ( unit->unit_Detached )
		{
			break;
		}

		if ( unit->unit_Buffer_Size )
		{
			Buffer_Pump( unit );

			Buffer_Schedule( unit );
		}
		else if ( unit->unit_Dot4_Active )
		{
			Dot4_Schedule( unit );
		}
		else
		{
			Direct_Schedule( unit );
		}

		if ( ! unit->unit_Detached )
		{
			Finish_Job( unit, closing && intake_drained );
		}
	}
}

// --
