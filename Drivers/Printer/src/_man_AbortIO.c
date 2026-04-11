
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

static void _manager_AbortIO( struct DeviceManagerInterface *Self UNUSED, struct IORequest *ioreq UNUSED )
{
// struct AbortIOMessage *msg;
// struct devUnit *unit;

	USBDEBUG( "Printer : _manager_AbortIO" );

// 	unit    = (APTR) ioreq->ios2_Req.io_Unit;

// 	// Only the required commands are aborted
//     switch( ioreq->ios2_Req.io_Command )
// 	{
// 		case CMD_READ:
// 		case CMD_WRITE:
// 		case S2_READORPHAN:
// 		case S2_BROADCAST:
// 		case S2_MULTICAST:
// 		case S2_ONEVENT:
// 		case S2_SAMPLE_THROUGHPUT:
// 		{
// 			if ( unit->unit_AbortIOMsgPort )
// 			{
// 				msg = IExec->AllocVecTags( sizeof( struct AbortIOMessage ),
// 					AVT_Clear, 0,
// 					TAG_END
// 				);

// 				if ( msg )
// 				{
// 					msg->ioreq = ioreq;

// 					// Forward request to device task
// 					IExec->PutMsg( unit->unit_AbortIOMsgPort, (APTR) msg );
// 				}
// 				else
// 				{
// 					ioreq->ios2_Req.io_Error = IOERR_UNITBUSY;

// 					IExec->ReplyMsg( (APTR) ioreq );
// 				}
// 			}
// 			else
// 			{
// 				ioreq->ios2_Req.io_Error = IOERR_UNITBUSY;

// 				IExec->ReplyMsg( (APTR) ioreq );
// 			}
// 			break;
// 		}

// 		default:
// 		{
// 			ioreq->ios2_Req.io_Error = IOERR_NOCMD;

// 			IExec->ReplyMsg( (APTR) ioreq );
// 			break;
// 		}
// 	}
}

// --
