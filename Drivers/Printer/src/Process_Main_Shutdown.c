
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

	while ( ( pb = (PTR)RemHead( unit->unit_Buffer_WaitingList ) ) )
	{
		FreeVec( pb );
	}

	unit->unit_Buffer_WaitingCount = 0;

	if ( unit->unit_Buffer_Active )
	{
		FreeVec( unit->unit_Buffer_Active );

		unit->unit_Buffer_Active = NULL;
	}

	while ( ( request = (PTR)RemHead( unit->unit_PAR_IOReq_WaitingList ) ) )
	{
		request->io_Error = ( request->io_Flags & IOPARF_ABORT ) ? IOERR_ABORTED : unit->unit_JobError;

		ReplyMsg( (PTR)request );
	}

	unit->unit_PAR_IOReq_WaitingCount = 0;
}

// --

static void
Close_Timeout( struct PrinterUnit *unit )
{
	struct USB2_IORequest *usbio;

	U32 index;

	unit->unit_CloseTimerPending = FALSE;

	if ( ! unit->unit_JobError )
	{
		unit->unit_JobError = ParErr_LineErr;
	}

	if ( unit->unit_CloseAborting )
	{
		// Unregister before cleanup. The worker still owns all unretired DMA.
		DebugPrintF( "USB2 printer: unit %lu did not retire its transfers\n", unit->unit_UnitNr );

		unit->unit_Detached = TRUE;

		return;
	}

	unit->unit_CloseAborting = TRUE;

	Discard_Job( unit );

	for ( index = 0; index < NR_of_TXs; index++ )
	{
		usbio = unit->unit_Bulk_Tx_Active[index];

		if ( usbio )
		{
			AbortIO( (PTR)usbio );
		}
	}

	Printer_CloseTimerArm( unit, 5 );
}

// --

static void
Finish_Job( struct PrinterUnit *unit )
{
	if ( unit->unit_JobError )
	{
		Discard_Job( unit );
	}

	if ( ! unit->unit_PortInterface->JobClosing( unit->unit_PortUnit ) )
	{
		return;
	}

	if ( ! unit->unit_CloseTimerPending )
	{
		Printer_CloseTimerArm( unit, 60 );
	}

	if ( ( unit->unit_Buffer_Size ) && ( ! unit->unit_JobError ) )
	{
		Buffer_Flush( unit );
	}

	if ( ( unit->unit_USB_IOReq_ActiveCount ) || ( unit->unit_PAR_IOReq_WaitingCount ) ||
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

	Printer_CloseTimerStop( unit );

	if ( unit->unit_JobError )
	{
		DebugPrintF( "USB2 printer: unit %lu job failed (%ld)\n", unit->unit_UnitNr, (LONG)unit->unit_JobError );
	}

	unit->unit_CloseAborting = FALSE;
	unit->unit_JobHasData = FALSE;
	unit->unit_NewJob = TRUE;

	unit->unit_PortInterface->FinishJob( unit->unit_PortUnit );
}

// --
