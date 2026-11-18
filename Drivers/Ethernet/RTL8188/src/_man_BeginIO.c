
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Manager.h"

// --

void
_manager_BeginIO( struct DeviceManagerInterface *Self UNUSED, struct IORequest *ioreq )
{
	struct EthernetUnit *unit;

	ioreq->io_Flags &= ~IOF_QUICK;
	ioreq->io_Message.mn_Node.ln_Type = NT_MESSAGE;

	if ( ioreq->io_Command == NSCMD_DEVICEQUERY )
	{
		if ( ioreq->io_Message.mn_Length >= sizeof( struct IOStdReq ) )
		{
			_cmd_4000_NSCmd_DeviceQuery( (PTR) ioreq );

			return;
		}

		ioreq->io_Error = IOERR_BADLENGTH;
	}
	else if ( ( ioreq->io_Command == CMD_INVALID ) &&
		( ioreq->io_Message.mn_Length < sizeof( struct IOSana2Req ) ) )
	{
		ioreq->io_Error = IOERR_NOCMD;
	}
	else if ( ioreq->io_Message.mn_Length < sizeof( struct IOSana2Req ) )
	{
		ioreq->io_Error = IOERR_BADLENGTH;
	}
	else
	{
		unit = (PTR) ioreq->io_Unit;

		if ( unit )
		{
			Sana2BeginIO( unit, (PTR) ioreq );

			return;
		}

		ioreq->io_Error = IOERR_UNITBUSY;
	}

	ReplyMsg( (PTR) ioreq );
}

// --
