
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
Discard_Job( struct PrinterUnit *unit )
{
	struct IOStdReq *request;
	struct PrtBuffer *pb;

	Printer_TimerStop( &unit->unit_IdleTimer );

	while ( ( pb = (PTR)RemHead( unit->unit_Buffer_WaitingList ) ) )
	{
		FreeVec( pb );
		unit->unit_Buffer_Allocated--;
	}

	unit->unit_Buffer_WaitingCount = 0;

	if ( unit->unit_Buffer_Active )
	{
		FreeVec( unit->unit_Buffer_Active );
		unit->unit_Buffer_Allocated--;

		unit->unit_Buffer_Active = NULL;
	}

	while ( ( request = (PTR)RemHead( unit->unit_PAR_IOReq_WaitingList ) ) )
	{
		request->io_Error = ( request->io_Flags & IOPARF_ABORT ) ? IOERR_ABORTED : unit->unit_JobError;

		Printer_Reply( (PTR)request );
	}

	unit->unit_PAR_IOReq_WaitingCount = 0;
}

// --

static void
Close_Timeout( struct PrinterUnit *unit )
{
	struct USB2_IORequest *usbio;

	U32 index;

	unit->unit_CloseTimer.Pending = FALSE;

	if ( ( ! unit->unit_Dot4_Request ) && ( ! unit->unit_USB_IOReq_ActiveCount ) && ( ! unit->unit_Buffer_Active ) &&
		( ! unit->unit_Buffer_WaitingCount ) && ( ! unit->unit_PAR_IOReq_WaitingCount ) )
	{
		return;
	}

	if ( ! unit->unit_JobError )
	{
		unit->unit_JobError = ParErr_LineErr;
	}

	if ( unit->unit_CloseAborting )
	{
		// Quiesce admission and retire DMA before releasing a waiting closer.
		DebugPrintF( "USB2 printer: unit %lu did not retire its transfers\n", unit->unit_UnitNr );

		unit->unit_Detached = TRUE;

		return;
	}

	unit->unit_CloseAborting = TRUE;

	Discard_Job( unit );

	Dot4_Abort( unit );

	for ( index = 0; index < NR_of_TXs; index++ )
	{
		usbio = unit->unit_Bulk_Tx_Active[index];

		if ( usbio )
		{
			AbortIO( (PTR)usbio );
		}
	}

	Printer_TimerArm( &unit->unit_CloseTimer, 5000000 );
}

// --

static void
Finish_Job( struct PrinterUnit *unit, S32 closing )
{
	if ( unit->unit_JobError )
	{
		Discard_Job( unit );
	}

	if ( ! closing )
	{
		return;
	}

	if ( ( unit->unit_Buffer_Active ) && ( ! unit->unit_JobError ) )
	{
		Printer_TimerStop( &unit->unit_IdleTimer );

		Buffer_Flush( unit );

		Buffer_Schedule( unit );
	}

	if ( ( unit->unit_Dot4_Request ) || ( unit->unit_USB_IOReq_ActiveCount ) || ( unit->unit_PAR_IOReq_WaitingCount ) ||
		( unit->unit_Buffer_WaitingCount ) )
	{
		return;
	}

	if ( ( ! unit->unit_JobError ) && ( ! unit->unit_Dot4_Active ) &&
		( unit->unit_JobHasData ) && ( unit->unit_LastByteSend != '\f' ) )
	{
		if ( Send_FormFeed( unit ) )
		{
			return;
		}

		unit->unit_JobError = ParErr_LineErr;
	}

	Printer_TimerStop( &unit->unit_CloseTimer );

	if ( unit->unit_JobError )
	{
		DebugPrintF( "USB2 printer: unit %lu job failed (%ld)\n", unit->unit_UnitNr, (LONG)unit->unit_JobError );
	}

	unit->unit_CloseAborting = FALSE;
	unit->unit_JobHasData = FALSE;
	unit->unit_JobError = 0;

	unit->unit_PortInterface->FinishJob( unit->unit_PortUnit );
}

// --
