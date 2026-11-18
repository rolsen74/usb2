/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Serial_Write_Done( struct SerialContext *ctx, struct SerialUnit *unit )
{
struct USB2_IORequest *usbio;
struct IOExtSer *ioreq;

	while(( usbio = (PTR) GetMsg( unit->BulkOut->MsgPort ) ))
	{
		unit->WritePending = FALSE;
		ioreq = unit->WriteActive;

		if ( ! ioreq )
		{
			continue;
		}

		if ( ioreq->IOSer.io_Flags & IOSERF_ABORT )
		{
			ioreq->IOSer.io_Error = IOERR_ABORTED;
			unit->WriteActive = NULL;
			Serial_Reply( ctx, ioreq );
			Serial_Write_Start( ctx, unit );
			continue;
		}

		if ( usbio->io_Error == USB2Err_NoError )
		{
			ioreq->IOSer.io_Actual += usbio->io_Actual;

			if ( ( usbio->io_Actual ) && ( ioreq->IOSer.io_Actual < ioreq->IOSer.io_Length ) )
			{
				Serial_Write_Start( ctx, unit );
				continue;
			}

			if ( ioreq->IOSer.io_Actual < ioreq->IOSer.io_Length )
			{
				ioreq->IOSer.io_Error = SerErr_DevBusy;
			}
		}
		else
		{
			ioreq->IOSer.io_Error = SerErr_DevBusy;

			if ( usbio->io_Error == USB2Err_Host_Stall )
			{
				USB2_EPRes_Destall( unit->BulkOut );
			}
			else
			{
				unit->Detached = TRUE;
			}
		}

		unit->WriteActive = NULL;
		Serial_Reply( ctx, ioreq );
		Serial_Write_Start( ctx, unit );
	}
}

// --
