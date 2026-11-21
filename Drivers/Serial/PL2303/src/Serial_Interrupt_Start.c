/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Serial_Interrupt_Start( struct SerialContext *ctx UNUSED, struct SerialUnit *unit )
{
struct USB2_IORequest *ioreq;

	if ( ( ! unit->Interrupt ) || ( unit->Detached ) || ( unit->InterruptPending ) )
	{
		return;
	}

	ioreq = unit->Interrupt->IORequests[0];
	ioreq->io_Command = CMD_READ;
	ioreq->io_Data = unit->Interrupt->Buffers[0];
	ioreq->io_Length = unit->Interrupt->BufferSize;
	ioreq->io_AllowShortPackets = TRUE;
	unit->InterruptPending = TRUE;

	SendIO( (PTR) ioreq );
}

// --
