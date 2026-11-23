
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

void
Send_FormFeed( struct PrinterUnit *unit )
{
	struct USB2_IORequest *usbio;

	U8 data[1];

	USBERROR( "Send_FormFeed" );

	data[0] = '\f';

	usbio = (PTR) RemHead( unit->unit_USB_IOReq_FreeList );

	if ( ! usbio )
	{
		return;
	}

	usbio->io_Command	= CMD_WRITE;
	usbio->io_UserData	= NULL;
	usbio->io_Length	= 1;
	usbio->io_Data		= data;

	DoIO( (PTR) usbio );

	if ( usbio->io_Error == USB2Err_Device_Detached )
	{
		unit->unit_Detached = TRUE;
	}

	AddTail( unit->unit_USB_IOReq_FreeList, (PTR) usbio );
}

// --
