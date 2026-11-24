/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Serial_Read_Done( struct SerialContext *ctx, struct SerialUnit *unit )
{
struct USB2_IORequest *ioreq;

U8 *data;
U32 index;
U32 next;

	while(( ioreq = (PTR) GetMsg( unit->BulkIn->MsgPort ) ))
	{
		unit->ReadPending = FALSE;

		if ( ioreq->io_Error == USB2Err_NoError )
		{
			unit->Recovery[SERIAL_PIPE_IN].Attempts = 0;
			data = ioreq->io_Data;

			for( index = 0; ( index < ioreq->io_Actual ) && ( index < ioreq->io_Length ); index++ )
			{
				next = ( unit->RxHead + 1 ) % SERIAL_RX_SIZE;

				if ( next == unit->RxTail )
				{
					unit->Overrun = TRUE;
					break;
				}

				unit->RxBuffer[unit->RxHead] = data[index];
				unit->RxHead = next;
			}

			Serial_Read_Drain( ctx, unit );
		}
		else
		{
			Serial_RecoveryError( ctx, unit, SERIAL_PIPE_IN, ioreq->io_Error );
		}
	}

	Serial_Read_Start( ctx, unit );
}

// --
