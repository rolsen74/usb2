
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

	U32 wait;
	U32 mask;

	reg = unit->unit_USB_Register;
	wait = SIGBREAKF_CTRL_C | reg->Stack_MsgPortBit | unit->unit_PRT_Begin_MsgPortBit |
		unit->unit_PRT_Abort_MsgPortBit | unit->unit_Bulk_Tx_Resource->MsgPortBit |
		( 1UL << unit->unit_CloseTimerPort->mp_SigBit );

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

		if ( ( mask & ( 1UL << unit->unit_CloseTimerPort->mp_SigBit ) ) &&
			( GetMsg( unit->unit_CloseTimerPort ) ) )
		{
			Close_Timeout( unit );

			if ( unit->unit_Detached )
			{
				break;
			}
		}

		if ( mask & unit->unit_PRT_Abort_MsgPortBit )
		{
			Printer_Abort( unit );
		}

		if ( mask & ( unit->unit_PRT_Begin_MsgPortBit | unit->unit_PRT_Abort_MsgPortBit ) )
		{
			__myProcess_Main_BeginIO( unit );
		}

		if ( ( ! unit->unit_Detached ) && ( mask & unit->unit_Bulk_Tx_Resource->MsgPortBit ) )
		{
			if ( unit->unit_Buffer_Size )
			{
				__myProcess_Main_Buffer( unit );
			}
			else
			{
				__myProcess_Main_Direct( unit );
			}
		}

		if ( ! unit->unit_Detached )
		{
			Finish_Job( unit );
		}
	}
}

// --
