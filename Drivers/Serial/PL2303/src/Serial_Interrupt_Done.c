/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Serial_Interrupt_Done( struct SerialContext *ctx, struct SerialUnit *unit )
{
struct USB2_IORequest *ioreq;

U8 *data;
U8 previous;

	while(( ioreq = (PTR) GetMsg( unit->Interrupt->MsgPort ) ))
	{
		unit->InterruptPending = FALSE;

		if ( ( ioreq->io_Error == USB2Err_NoError ) && ( unit->ACM ) &&
			( ioreq->io_Actual <= ioreq->io_Length ) )
		{
			Serial_ACM_Notification( ctx, unit, ioreq->io_Data, ioreq->io_Actual );
		}
		else if ( ( ioreq->io_Error == USB2Err_NoError ) && ( ! unit->ACM ) &&
			( ioreq->io_Actual > SERIAL_UART_INDEX ) )
		{
			data = ioreq->io_Data;
			previous = unit->UartState;
			unit->UartState = data[SERIAL_UART_INDEX];

			if ( ( unit->UartState & SERIAL_UART_BREAK ) && ( ! ( previous & SERIAL_UART_BREAK ) ) )
			{
				Serial_Read_Break( ctx, unit );
			}
		}
		else if ( ioreq->io_Error == USB2Err_Host_Stall )
		{
			unit->ACMNotificationUsed = 0;
			unit->ACMNotificationSize = 0;

			USB2_EPRes_Destall( unit->Interrupt );
		}
		else if ( ioreq->io_Error != USB2Err_NoError )
		{
			unit->Detached = TRUE;
		}
	}

	Serial_Interrupt_Start( ctx, unit );
}

// --
