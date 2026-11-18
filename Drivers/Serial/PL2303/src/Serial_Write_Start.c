/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Serial_Write_Start( struct SerialContext *ctx UNUSED, struct SerialUnit *unit )
{
struct USB2_IORequest *usbio;
struct IOExtSer *ioreq;

	if ( ( unit->Detached ) || ( unit->WritePending ) || ( unit->Stopped ) )
	{
		return;
	}

	if ( ! unit->WriteActive )
	{
		unit->WriteActive = (PTR) RemHead( & unit->WriteList );
	}

	ioreq = unit->WriteActive;

	if ( ! ioreq )
	{
		return;
	}

	usbio = unit->BulkOut->IORequests[0];
	usbio->io_Command = CMD_WRITE;
	usbio->io_Data = (U8 *) ioreq->IOSer.io_Data + ioreq->IOSer.io_Actual;
	usbio->io_Length = ioreq->IOSer.io_Length - ioreq->IOSer.io_Actual;
	usbio->io_UserData = ioreq;
	unit->WritePending = TRUE;

	SendIO( (PTR) usbio );
}

// --
