
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

void
Buffer_Pump( struct PrinterUnit *unit )
{
	struct IOStdReq *request;

	U32 count;
	U32 length;
	U32 accepted;
	U32 index;

	if ( ( unit->unit_Detached ) || ( unit->unit_JobError ) )
	{
		return;
	}

	count = 0;

	for ( index = 0 ; index < 32 ; index++ )
	{
		request = (PTR)GetHead( unit->unit_PAR_IOReq_WaitingList );

		if ( ! request )
		{
			break;
		}

		if ( ! ( request->io_Flags & IOPARF_ABORT ) )
		{
			if ( ! request->io_Actual )
			{
				unit->unit_Buffer_WriteLength = ( request->io_Length == -1U )
					? strlen( request->io_Data ) : request->io_Length;
			}

			length = unit->unit_Buffer_WriteLength;
			accepted = 0;

			if ( request->io_Actual < length )
			{
				accepted = Buffer_Write( unit, (U8 *)request->io_Data + request->io_Actual,
					length - request->io_Actual );

				request->io_Actual += accepted;
				count += accepted;
			}

			if ( ( request->io_Actual < length ) && ( ! unit->unit_JobError ) )
			{
				break;
			}
		}

		Remove( (PTR)request );

		unit->unit_PAR_IOReq_WaitingCount--;
		request->io_Error = unit->unit_JobError;

		Printer_Reply( (PTR)request );

		if ( unit->unit_JobError )
		{
			break;
		}
	}

	if ( index == 32 )
	{
		Signal( FindTask( NULL ), unit->unit_PRT_Begin_MsgPortBit );
	}

	if ( count )
	{
		Printer_TimerStop( &unit->unit_IdleTimer );

		if ( ( unit->unit_Buffer_Active ) && ( ! unit->unit_JobError ) )
		{
			Printer_TimerArm( &unit->unit_IdleTimer, PRINTER_IDLE_US );
		}
	}
}

// --
