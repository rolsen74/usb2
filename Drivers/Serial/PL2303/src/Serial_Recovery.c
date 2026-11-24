
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

#define SERIAL_RECOVERY_ATTEMPTS 3

SEC_CODE S32
Serial_RecoveryInit( struct SerialUnit *unit )
{
	unit->RecoveryPort = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! unit->RecoveryPort )
	{
		return ( FALSE );
	}

	unit->RecoveryTimer = AllocSysObjectTags( ASOT_IOREQUEST,
		ASOIOR_Size, sizeof( *unit->RecoveryTimer ), ASOIOR_ReplyPort, unit->RecoveryPort, TAG_END );

	if ( ! unit->RecoveryTimer )
	{
		return ( FALSE );
	}

	unit->RecoveryOpen = ( OpenDevice( "timer.device", UNIT_MICROHZ, (PTR)unit->RecoveryTimer, 0 ) == 0 );

	return ( unit->RecoveryOpen );
}

// --

SEC_CODE void
Serial_RecoveryFree( struct SerialUnit *unit )
{
	if ( unit->RecoveryPending )
	{
		if ( ! CheckIO( (PTR)unit->RecoveryTimer ) )
		{
			AbortIO( (PTR)unit->RecoveryTimer );
		}

		WaitIO( (PTR)unit->RecoveryTimer );
	}

	if ( unit->RecoveryOpen )
	{
		CloseDevice( (PTR)unit->RecoveryTimer );
	}

	if ( unit->RecoveryTimer )
	{
		FreeSysObject( ASOT_IOREQUEST, unit->RecoveryTimer );
	}

	if ( unit->RecoveryPort )
	{
		FreeSysObject( ASOT_PORT, unit->RecoveryPort );
	}
}

// --

SEC_CODE static void
Schedule( struct SerialUnit *unit )
{
	if ( ( unit->RecoveryPending ) || ( unit->Detached ) )
	{
		return;
	}

	unit->RecoveryTimer->Request.io_Command = TR_ADDREQUEST;
	unit->RecoveryTimer->Time.Seconds = 0;
	unit->RecoveryTimer->Time.Microseconds = 100000;
	unit->RecoveryPending = TRUE;

	SendIO( (PTR)unit->RecoveryTimer );
}

// --

SEC_CODE static void
FailReads( struct SerialContext *ctx, struct SerialUnit *unit )
{
	struct IOExtSer *ioreq;

	ioreq = unit->ReadActive;
	unit->ReadActive = NULL;

	if ( ioreq )
	{
		ioreq->IOSer.io_Error = SerErr_DevBusy;

		Serial_Reply( ctx, ioreq );
	}

	while(( ioreq = (PTR)RemHead( &unit->ReadList ) ))
	{
		ioreq->IOSer.io_Error = SerErr_DevBusy;

		Serial_Reply( ctx, ioreq );
	}
}

// --

SEC_CODE void
Serial_RecoveryError( struct SerialContext *ctx, struct SerialUnit *unit, enum SerialPipe pipe, S32 error )
{
	struct SerialPipeRecovery *recovery;

	if ( ( error == USB2Err_Device_Detached ) || ( error == USB2Err_Device_OwnershipChange ) )
	{
		unit->Detached = TRUE;

		return;
	}

	if ( ( unit->Detached ) || ( error == IOERR_ABORTED ) ||
		( ( pipe == SERIAL_PIPE_OUT ) && ( error != USB2Err_Host_Stall ) ) )
	{
		return;
	}

	recovery = &unit->Recovery[pipe];

	if ( ! recovery->Attempts )
	{
		DebugPrintF( "USB2 serial: pipe %lu recovery after USB error %ld\n", (U32)pipe, error );
	}

	if ( error == USB2Err_Host_Stall )
	{
		recovery->Stalled = TRUE;
	}

	if ( recovery->Attempts == SERIAL_RECOVERY_ATTEMPTS )
	{
		recovery->Attempts = 0;
		recovery->Failed = TRUE;
		recovery->Waiting = FALSE;

		DebugPrintF( "USB2 serial: pipe %lu recovery exhausted\n", (U32)pipe );

		if ( pipe == SERIAL_PIPE_IN )
		{
			FailReads( ctx, unit );
		}
		else if ( pipe == SERIAL_PIPE_STATUS )
		{
			unit->UartState = 0;
		}

		return;
	}

	recovery->Attempts++;
	recovery->Waiting = TRUE;

	Schedule( unit );
}

// --

SEC_CODE static void
RecoverPipe( struct SerialContext *ctx, struct SerialUnit *unit, enum SerialPipe pipe, struct USB2_EPResource *ep )
{
	struct SerialPipeRecovery *recovery;

	recovery = &unit->Recovery[pipe];

	if ( ( ! recovery->Waiting ) || ( ! ep ) )
	{
		return;
	}

	if ( recovery->Stalled )
	{
		if ( ! USB2_EPRes_Destall( ep ) )
		{
			Serial_RecoveryError( ctx, unit, pipe, USB2Err_Host_Stall );

			return;
		}

		recovery->Stalled = FALSE;
	}

	recovery->Waiting = FALSE;

	// Keep the error count until an actual USB transfer succeeds.
	if ( pipe == SERIAL_PIPE_IN )
	{
		Serial_Read_Start( ctx, unit );
	}
	else if ( pipe == SERIAL_PIPE_OUT )
	{
		Serial_Write_Start( ctx, unit );
	}
	else
	{
		Serial_Interrupt_Start( ctx, unit );
	}
}

// --

SEC_CODE void
Serial_RecoveryDone( struct SerialContext *ctx, struct SerialUnit *unit )
{
	if ( ! GetMsg( unit->RecoveryPort ) )
	{
		return;
	}

	unit->RecoveryPending = FALSE;

	RecoverPipe( ctx, unit, SERIAL_PIPE_IN, unit->BulkIn );

	RecoverPipe( ctx, unit, SERIAL_PIPE_OUT, unit->BulkOut );

	RecoverPipe( ctx, unit, SERIAL_PIPE_STATUS, unit->Interrupt );

	Serial_Write_Start( ctx, unit );
}

SEC_CODE void
Serial_RecoveryResume( struct SerialUnit *unit )
{
	struct SerialPipeRecovery *recovery;

	U32 pipe;

	for( pipe = 0; pipe < SERIAL_PIPE_COUNT; pipe++ )
	{
		recovery = &unit->Recovery[pipe];
		recovery->Failed = FALSE;
		recovery->Attempts = 0;
		recovery->Waiting = recovery->Stalled;

		if ( recovery->Waiting )
		{
			recovery->Attempts = 1;

			Schedule( unit );
		}
	}
}

// --
