/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void
Serial_Write_Account( struct SerialUnit *unit, struct USB2_IORequest *usbio )
{
	struct IOExtSer *ioreq;

	U32 confirmed;
	U32 remaining;

	ioreq = unit->WriteActive;

	if ( ( ioreq ) && ( ioreq->IOSer.io_Actual < unit->WriteLength ) )
	{
		remaining = unit->WriteLength - ioreq->IOSer.io_Actual;
		confirmed = ( usbio->io_Actual < usbio->io_Length ) ? usbio->io_Actual : usbio->io_Length;
		ioreq->IOSer.io_Actual += ( confirmed < remaining ) ? confirmed : remaining;
	}
}

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

		Serial_Write_Account( unit, usbio );

		if ( usbio->io_Error == USB2Err_NoError )
		{
			unit->Recovery[SERIAL_PIPE_OUT].Attempts = 0;
		}
		else
		{
			Serial_RecoveryError( ctx, unit, SERIAL_PIPE_OUT, usbio->io_Error );
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
			if ( ( usbio->io_Actual ) && ( ioreq->IOSer.io_Actual < unit->WriteLength ) )
			{
				Serial_Write_Start( ctx, unit );
				continue;
			}

			if ( ioreq->IOSer.io_Actual < unit->WriteLength )
			{
				ioreq->IOSer.io_Error = SerErr_DevBusy;
			}
		}
		else
		{
			ioreq->IOSer.io_Error = SerErr_DevBusy;
		}

		unit->WriteActive = NULL;
		Serial_Reply( ctx, ioreq );
		Serial_Write_Start( ctx, unit );
	}
}

// --
