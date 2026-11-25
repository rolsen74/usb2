/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Audio_Manager_BeginIO( struct DeviceManagerInterface *Self, struct IOStdReq *ioreq )
{
struct AudioBase *base;
struct AudioUnit *unit;

	base = (PTR) Self->Data.LibBase;
	unit = (PTR) ioreq->io_Unit;

	if ( ioreq->io_Message.mn_Length < sizeof( *ioreq ) )
	{
		ioreq->io_Error = IOERR_BADLENGTH;

		if ( ! ( ioreq->io_Flags & IOF_QUICK ) )
		{
			ioreq->io_Message.mn_Node.ln_Type = NT_MESSAGE;

			ReplyMsg( (PTR) ioreq );
		}

		return;
	}

	if ( ioreq->io_Command == AUDIO_CMD_ENUM_ENDPOINTS )
	{
		Audio_EndpointList( base, ioreq );

		if ( ! ( ioreq->io_Flags & IOF_QUICK ) )
		{
			ReplyMsg( (PTR) ioreq );
		}

		return;
	}

	ioreq->io_Flags &= ~( IOF_QUICK | AUDIO_IO_ABORT );
	ioreq->io_Error = 0;
	ioreq->io_Actual = 0;
	ioreq->io_Message.mn_Node.ln_Type = NT_MESSAGE;

	ObtainSemaphore( & base->Semaphore );

	if ( ( ! unit ) || ( unit->Detached ) )
	{
		ioreq->io_Error = unit ? USB2Err_Device_Detached : IOERR_OPENFAIL;
		ReleaseSemaphore( & base->Semaphore );
		ReplyMsg( (PTR) ioreq );

		return;
	}

	PutMsg( unit->BeginPort, (PTR) ioreq );
	ReleaseSemaphore( & base->Semaphore );
}

// --
