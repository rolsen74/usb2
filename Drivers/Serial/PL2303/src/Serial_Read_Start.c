/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Serial_Read_Start( struct SerialContext *ctx UNUSED, struct SerialUnit *unit )
{
struct USB2_IORequest *ioreq;

U32 freebytes;

	if ( ( unit->Detached ) || ( unit->ReadPending ) )
	{
		return;
	}

	freebytes = ( unit->RxTail + SERIAL_RX_SIZE - unit->RxHead - 1 ) % SERIAL_RX_SIZE;

	if ( freebytes < unit->BulkIn->BufferSize )
	{
		return;
	}

	ioreq = unit->BulkIn->IORequests[0];
	ioreq->io_Command = CMD_READ;
	ioreq->io_Data = unit->BulkIn->Buffers[0];
	ioreq->io_Length = unit->BulkIn->BufferSize;
	ioreq->io_AllowShortPackets = TRUE;
	unit->ReadPending = TRUE;

	SendIO( (PTR) ioreq );
}

// --
