/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Serial_Write_Start( struct SerialContext *ctx, struct SerialUnit *unit )
{
struct USB2_IORequest *usbio;
struct IOExtSer *ioreq;

	while ( ( ! unit->Detached ) && ( ! unit->WritePending ) && ( ! unit->Stopped ) && ( ! unit->BreakActive ) )
	{
		if ( ! unit->WriteActive )
		{
			unit->WriteActive = (PTR)RemHead( &unit->WriteList );
		}

		ioreq = unit->WriteActive;

		if ( ! ioreq )
		{
			return;
		}

		if ( ioreq->IOSer.io_Flags & IOSERF_ABORT )
		{
			unit->WriteActive = NULL;

			Serial_Reply( ctx, ioreq );

			continue;
		}

		if ( ioreq->IOSer.io_Command == SDCMD_BREAK )
		{
			unit->WriteActive = NULL;

			Serial_BreakStart( ctx, unit, ioreq );

			continue;
		}

		usbio = unit->BulkOut->IORequests[0];
		usbio->io_Command = CMD_WRITE;
		usbio->io_Data = (U8 *)ioreq->IOSer.io_Data + ioreq->IOSer.io_Actual;
		usbio->io_Length = ioreq->IOSer.io_Length - ioreq->IOSer.io_Actual;
		usbio->io_UserData = ioreq;
		unit->WritePending = TRUE;

		SendIO( (PTR)usbio );
	}
}

// --
