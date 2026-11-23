
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

S32
Send_FormFeed( struct PrinterUnit *unit )
{
	struct USB2_IORequest *usbio;

	usbio = (PTR)RemHead( unit->unit_USB_IOReq_FreeList );

	if ( ! usbio )
	{
		return ( FALSE );
	}

	unit->unit_FormFeed = '\f';
	unit->unit_LastByteSend = '\f';
	unit->unit_Bulk_Tx_Active[usbio->io_UserValue] = usbio;
	unit->unit_USB_IOReq_ActiveCount++;

	usbio->io_Command = CMD_WRITE;
	usbio->io_UserData = NULL;
	usbio->io_Length = 1;
	usbio->io_Data = &unit->unit_FormFeed;

	SendIO( (PTR)usbio );

	return ( TRUE );
}

// --

void
Printer_FormFeedDone( struct PrinterUnit *unit, struct USB2_IORequest *usbio )
{
	if ( usbio->io_Error == USB2Err_Device_Detached )
	{
		unit->unit_Detached = TRUE;
	}
	else if ( ( ! unit->unit_JobError ) && ( ( usbio->io_Error ) || ( usbio->io_Actual != 1 ) ) )
	{
		unit->unit_JobError = ParErr_LineErr;
	}

	AddTail( unit->unit_USB_IOReq_FreeList, (PTR)usbio );
}

// --
