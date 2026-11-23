
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include <devices/newstyle.h>

// --

SEC_CODE void
USB2_DeviceQuery( struct IOStdReq *ioreq, U16 type, const U16 *commands )
{
	struct NSDeviceQueryResult *result;

	if ( ioreq->io_Message.mn_Length < sizeof( *ioreq ) )
	{
		ioreq->io_Error = IOERR_BADLENGTH;

		return;
	}

	result = ioreq->io_Data;
	ioreq->io_Actual = 0;

	if ( ( ! result ) || ( ioreq->io_Length < sizeof( *result ) ) )
	{
		ioreq->io_Error = IOERR_BADLENGTH;

		return;
	}

	result->DevQueryFormat = 0;
	result->SizeAvailable = sizeof( *result );
	result->DeviceType = type;
	result->DeviceSubType = 0;
	result->SupportedCommands = (PTR) commands;
	ioreq->io_Actual = sizeof( *result );
	ioreq->io_Error = 0;
}

// --
