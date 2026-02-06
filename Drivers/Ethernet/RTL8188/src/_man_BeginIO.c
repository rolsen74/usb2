
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

static void _manager_BeginIO( struct DeviceManagerInterface *Self UNUSED, struct IORequest *ioreq )
{
struct EthernetUnit *unit;

	USBDEBUG( "RTL8188 : _manager_BeginIO : IOReq %p", ioreq );

	// Disable QUICK IO - we do not support that
	ioreq->io_Flags &= ~IOF_QUICK;
	ioreq->io_Error = 0;

//	ioreq->req_Public.io_Message.mn_Node.ln_Type = NT_MESSAGE;
//	ioreq->req_Public.io_Flags	= 0;
//	ioreq->req_Public.io_Error	= 0;
//	ioreq->req_Public.io_Actual	= 0;

	switch( ioreq->io_Command )
	{
		case /* 4000 */ NSCMD_DEVICEQUERY:
		{
			_cmd_4000_NSCmd_DeviceQuery( (PTR) ioreq );
			break;
		}

		case /* 0003 */	CMD_WRITE:
		{
			unit = (PTR) ioreq->io_Unit;

			if (( ! unit->unit_Shutdown ) && ( ! unit->unit_StartupComplete ))
			{
				Unit_Startup( unit );
			}

			if (( ! unit->unit_Shutdown ) && ( unit->unit_StartupComplete ))
			{
				PutMsg( unit->unit_ETH_Begin_MsgPort, (PTR) ioreq );
			}
			else
			{
				ioreq->io_Error = IOERR_UNITBUSY;
				ReplyMsg( (PTR) ioreq );
			}
			break;
		}

		default:
		{
			USBDEBUG( "RTL8188 : _manager_BeginIO : Unknown Cmd ($%04lx)", (U32) ioreq->io_Command );

			// Unknown Command
			ioreq->io_Error = IOERR_NOCMD;

			ReplyMsg( (PTR) ioreq );
			break;
		}
	}
}

// --
