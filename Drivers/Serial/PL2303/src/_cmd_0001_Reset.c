/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE S32
Serial_Reset( struct SerialContext *ctx, struct SerialUnit *unit )
{
	struct IOExtSer *ioreq;
	struct IOExtSer defaults;

	Serial_Flush( ctx, unit, TRUE );

	Serial_BreakStop( ctx, unit, IOERR_ABORTED );

	if ( unit->ReadPending )
	{
		Serial_IO_Stop( ctx, unit->BulkIn->IORequests[0] );

		unit->ReadPending = FALSE;
	}

	if ( unit->WritePending )
	{
		Serial_IO_Stop( ctx, unit->BulkOut->IORequests[0] );

		Serial_Write_Account( unit, unit->BulkOut->IORequests[0] );

		unit->WritePending = FALSE;
	}

	if ( unit->InterruptPending )
	{
		Serial_IO_Stop( ctx, unit->Interrupt->IORequests[0] );

		unit->InterruptPending = FALSE;
	}

	ioreq = unit->WriteActive;
	unit->WriteActive = NULL;

	if ( ioreq )
	{
		ioreq->IOSer.io_Error = IOERR_ABORTED;

		Serial_Reply( ctx, ioreq );
	}

	unit->RxHead = 0;
	unit->RxTail = 0;
	unit->Stopped = FALSE;
	unit->UartState = 0;
	unit->Overrun = FALSE;
	unit->BreakQuery = FALSE;
	unit->ReadBreaks = 0;
	unit->ACMNotificationUsed = 0;
	unit->ACMNotificationSize = 0;

	if ( ! Serial_DefaultLine( ctx, unit ) )
	{
		return ( FALSE );
	}

	if ( unit->Sessions )
	{
		defaults.io_Baud = 9600;
		defaults.io_ReadLen = 8;
		defaults.io_WriteLen = 8;
		defaults.io_StopBits = 1;
		defaults.io_SerFlags = 0;
		defaults.io_ExtFlags = 0;

		unit->PortInterface->SetParams( unit->PortUnit, &defaults );
	}

	return ( ! unit->BreakActive );
}

// --

SEC_CODE void
Serial_Cmd_Reset( struct SerialContext *ctx, struct SerialUnit *unit, struct IOExtSer *ioreq )
{
	if ( Serial_Reset( ctx, unit ) )
	{
		ioreq->io_Baud = 9600;
		ioreq->io_ReadLen = 8;
		ioreq->io_WriteLen = 8;
		ioreq->io_StopBits = 1;
		ioreq->io_SerFlags &= ~( SERF_PARTY_ON | SERF_PARTY_ODD );
		ioreq->io_ExtFlags = 0;
		ioreq->io_RBufLen = SERIAL_RX_SIZE;
		ioreq->io_BrkTime = 250000;
		ioreq->io_CtlChar = SER_DEFAULT_CTLCHAR;
		ioreq->io_TermArray.TermArray0 = 0;
		ioreq->io_TermArray.TermArray1 = 0;
	}
	else
	{
		ioreq->IOSer.io_Error = SerErr_DevBusy;
	}

	Serial_RecoveryResume( unit );

	Serial_Read_Start( ctx, unit );

	Serial_Interrupt_Start( ctx, unit );

	Serial_Reply( ctx, ioreq );
}

// --
