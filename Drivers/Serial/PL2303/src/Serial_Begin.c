/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Serial_Begin( struct SerialContext *ctx, struct SerialUnit *unit )
{
struct IOExtSer *ioreq;

	while(( ioreq = (PTR) GetMsg( unit->BeginPort ) ))
	{
		if ( unit->Detached )
		{
			ioreq->IOSer.io_Error = IOERR_UNITBUSY;
			Serial_Reply( ctx, ioreq );
			continue;
		}

		if ( ioreq->IOSer.io_Flags & IOSERF_ABORT )
		{
			ioreq->IOSer.io_Error = IOERR_ABORTED;
			Serial_Reply( ctx, ioreq );
			continue;
		}

		switch( ioreq->IOSer.io_Command )
		{
			case CMD_READ:           Serial_Cmd_Read( ctx, unit, ioreq ); break;
			case CMD_WRITE:          Serial_Cmd_Write( ctx, unit, ioreq ); break;
			case CMD_RESET:          Serial_Cmd_Reset( ctx, unit, ioreq ); break;
			case CMD_FLUSH:          Serial_Cmd_Flush( ctx, unit, ioreq ); break;
			case CMD_CLEAR:          Serial_Cmd_Clear( ctx, unit, ioreq ); break;
			case CMD_STOP:           Serial_Cmd_Stop( ctx, unit, ioreq ); break;
			case CMD_START:          Serial_Cmd_Start( ctx, unit, ioreq ); break;
			case SDCMD_SETPARAMS:    Serial_Cmd_SetParams( ctx, unit, ioreq ); break;
			case SDCMD_QUERY:        Serial_Cmd_Query( ctx, unit, ioreq ); break;
			case SDCMD_BREAK:        Serial_Cmd_Break( ctx, unit, ioreq ); break;
			case NSCMD_DEVICEQUERY:  Serial_Cmd_DeviceQuery( ctx, unit, (PTR) ioreq ); break;

			default:
			{
				ioreq->IOSer.io_Error = IOERR_NOCMD;
				Serial_Reply( ctx, ioreq );
				break;
			}
		}
	}
}

// --
