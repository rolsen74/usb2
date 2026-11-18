/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Serial_Cmd_Query( struct SerialContext *ctx, struct SerialUnit *unit, struct IOExtSer *ioreq )
{
U16 status;
U8 state;

	state = unit->UartState;
	status = 0;

	if ( state & SERIAL_UART_RING )
	{
		status |= ( 1 << 2 );
	}

	if ( ! ( state & SERIAL_UART_DSR ) )
	{
		status |= ( 1 << 3 );
	}

	if ( ! ( state & SERIAL_UART_CTS ) )
	{
		status |= ( 1 << 4 );
	}

	if ( ! ( state & SERIAL_UART_DCD ) )
	{
		status |= ( 1 << 5 );
	}

	if ( unit->Overrun || ( state & ( SERIAL_UART_FRAME | SERIAL_UART_PARITY | SERIAL_UART_OVERRUN ) ) )
	{
		status |= IO_STATF_OVERRUN;
	}

	if ( state & SERIAL_UART_BREAK )
	{
		status |= IO_STATF_READBREAK;
	}

	ioreq->io_Status = status;
	ioreq->IOSer.io_Actual = ( unit->RxHead + SERIAL_RX_SIZE - unit->RxTail ) % SERIAL_RX_SIZE;
	unit->Overrun = FALSE;

	Serial_Reply( ctx, ioreq );
}

// --
