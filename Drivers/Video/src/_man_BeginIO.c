/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"
#include <interfaces/usb2units.h>

// --

SEC_CODE void Video_Manager_BeginIO( struct DeviceManagerInterface *Self, struct IOStdReq *ioreq )
{
struct VideoBase *base;
struct VideoSlot *slot;
struct VideoUnit *unit;

	base = (PTR) Self->Data.LibBase;
	slot = (PTR) ioreq->io_Unit;
	if ( ioreq->io_Command == USB2_CMD_ENUM_UNITS )
	{
		if ( ioreq->io_Message.mn_Length < sizeof( *ioreq ) )
		{
			ioreq->io_Error = IOERR_BADLENGTH;
		}
		else
		{
			ioreq->io_Error = 0;
			ioreq->io_Actual = 0;

			Video_EnumUnits( base, ioreq );
		}

		if ( ! ( ioreq->io_Flags & IOF_QUICK ) )
		{
			ReplyMsg( (PTR) ioreq );
		}

		return;
	}

	ioreq->io_Flags &= ~( IOF_QUICK | VIDEO_IO_ABORT );
	ioreq->io_Error = 0;
	ioreq->io_Actual = 0;
	ioreq->io_Message.mn_Node.ln_Type = NT_MESSAGE;

	ObtainSemaphore( & base->Semaphore );

	unit = ( slot && slot->Open ) ? slot->Unit : NULL;

	if ( ! unit )
	{
		ioreq->io_Error = ( slot && slot->Open ) ? USB2Err_Device_Detached : IOERR_UNITBUSY;
		ReleaseSemaphore( & base->Semaphore );
		ReplyMsg( (PTR) ioreq );

		return;
	}

	PutMsg( unit->BeginPort, (PTR) ioreq );
	ReleaseSemaphore( & base->Semaphore );
}

// --
