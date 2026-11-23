
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

static void
__myProcess_Main_Buffer( struct PrinterUnit *unit )
{
	struct USB2_IORequest *usbio;
	struct PrtBuffer *pb;

	while ( ( usbio = (PTR)GetMsg( unit->unit_Bulk_Tx_Resource->MsgPort ) ) )
	{
		pb = usbio->io_UserData;
		unit->unit_Bulk_Tx_Active[usbio->io_UserValue] = NULL;
		unit->unit_USB_IOReq_ActiveCount--;

		if ( ! pb )
		{
			Printer_FormFeedDone( unit, usbio );

			continue;
		}

		AddTail( unit->unit_USB_IOReq_FreeList, (PTR)usbio );

		if ( usbio->io_Error == USB2Err_Device_Detached )
		{
			unit->unit_Detached = TRUE;
		}
		else if ( ( ! usbio->io_Error ) && ( usbio->io_Actual ) && ( usbio->io_Actual <= usbio->io_Length ) )
		{
			pb->pb_Sent += usbio->io_Actual;
			unit->unit_LastByteSend = pb->pb_Data[pb->pb_Sent - 1];
			unit->unit_JobHasData = TRUE;

			if ( ( pb->pb_Sent < pb->pb_Pos ) && ( ! unit->unit_JobError ) )
			{
				AddHead( unit->unit_Buffer_WaitingList, &pb->pb_Node );

				unit->unit_Buffer_WaitingCount++;

				continue;
			}
		}
		else if ( ! unit->unit_JobError )
		{
			unit->unit_JobError = ParErr_LineErr;

			if ( ( usbio->io_Error == USB2Err_Host_Stall ) && ( ! USB2_EPRes_Destall( unit->unit_Bulk_Tx_Resource ) ) )
			{
				unit->unit_Detached = TRUE;
			}
		}

		if ( unit->unit_Buffer_FreeCount < 3 )
		{
			AddTail( unit->unit_Buffer_FreeList, &pb->pb_Node );

			unit->unit_Buffer_FreeCount++;
		}
		else
		{
			FreeVec( pb );
		}
	}

	Buffer_Schedule( unit );
}

// --
