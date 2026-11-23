
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
Buffer_Schedule( struct PrinterUnit *unit )
{
	struct USB2_IORequest *usbio;
	struct PrtBuffer *pb;

	if ( ( unit->unit_Detached ) || ( unit->unit_JobError ) || ( unit->unit_USB_IOReq_ActiveCount ) )
	{
		return;
	}

	usbio = (PTR)GetHead( unit->unit_USB_IOReq_FreeList );

	pb = (PTR)GetHead( unit->unit_Buffer_WaitingList );

	if ( ( ! usbio ) || ( ! pb ) )
	{
		return;
	}

	Remove( (PTR)pb );

	Remove( (PTR)usbio );

	unit->unit_Bulk_Tx_Active[usbio->io_UserValue] = usbio;
	unit->unit_USB_IOReq_ActiveCount++;
	unit->unit_Buffer_WaitingCount--;

	usbio->io_Command = CMD_WRITE;
	usbio->io_UserData = pb;
	usbio->io_Length = pb->pb_Pos - pb->pb_Sent;
	usbio->io_Data = pb->pb_Data + pb->pb_Sent;

	SendIO( (PTR)usbio );
}

// --
