
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Radio.h"
#include "Registers.h"

// --

S32
RTL8821CU_DrainTX( struct EthernetUnit *unit )
{
	struct USB2_EPResource *control;

	U32 attempt;
	U32 queue;
	U32 mask;
	U32 reserved;
	U32 available;
	U32 empty;
	U32 status;
	U16 address;

	control = unit->unit_USB_Register->Res_Control;

	if ( unit->unit_TxQueued )
	{
		for( attempt = 0; ( attempt < 5 ) && ( unit->unit_TxQueued ); attempt++ )
		{
			for( queue = 0; queue < RTL8821CU_TX_PIPES; queue++ )
			{
				mask = 1U << queue;

				if ( ! ( unit->unit_TxQueued & mask ) )
				{
					continue;
				}

				address = REG_FIFOPAGE_INFO_1 + queue * 4;

				if ( ( ! RTL8821CU_ReadReg( control, address, 2, & reserved ) ) ||
					( ! RTL8821CU_ReadReg( control, address + 2, 2, & available ) ) )
				{
					return ( FALSE );
				}

				if ( reserved == available )
				{
					unit->unit_TxQueued &= ~mask;
				}
				else if ( attempt == 4 )
				{
					DebugPrintF( "RTL8821CU: TX queue %lu has %lu/%lu free pages\n", queue, available, reserved );
				}
			}

			if ( ( unit->unit_TxQueued ) && ( attempt != 4 ) )
			{
				Delay( 1 );
			}
		}

		if ( unit->unit_TxQueued )
		{
			if ( ( RTL8821CU_ReadReg( control, REG_TXPKT_EMPTY, 2, & empty ) ) &&
				( RTL8821CU_ReadReg( control, REG_TXDMA_STATUS, 4, & status ) ) )
			{
				DebugPrintF( "RTL8821CU: TX drain timeout, empty $%04lx, DMA status $%08lx\n", empty, status );
			}

			return ( FALSE );
		}
	}

	if ( ! RTL8821CU_WriteReg( control, REG_TXPAUSE, 1, 0xff ) )
	{
		return ( FALSE );
	}

	unit->unit_TxReady = FALSE;

	return ( TRUE );
}

// --
