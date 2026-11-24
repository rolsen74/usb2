/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE static U32
WriteLength( struct IOExtSer *ioreq )
{
	U8 *data;
	U8 *terms;
	U32 length;
	U32 index;
	U32 term;

	data = ioreq->IOSer.io_Data;
	length = ioreq->IOSer.io_Length;
	terms = (PTR) &ioreq->io_TermArray;

	if ( ( length != (U32)-1 ) && ( ! ( ioreq->io_SerFlags & SERF_EOFMODE ) ) )
	{
		return ( length );
	}

	for( index = 0; index < length; index++ )
	{
		if ( ( length == (U32)-1 ) && ( ! data[index] ) )
		{
			return ( index + 1 );
		}

		if ( ioreq->io_SerFlags & SERF_EOFMODE )
		{
			for( term = 0; term < sizeof( ioreq->io_TermArray ); term++ )
			{
				if ( data[index] == terms[term] )
				{
					return ( index + 1 );
				}
			}
		}
	}

	return ( length );
}

// --

SEC_CODE void
Serial_Write_Start( struct SerialContext *ctx, struct SerialUnit *unit )
{
	struct USB2_IORequest *usbio;
	struct IOExtSer *ioreq;

	U32 remaining;

	while( ( unit->SessionOpen ) && ( ! unit->Detached ) && ( ! unit->WritePending ) && ( ! unit->Stopped ) &&
		( ( ! unit->BreakActive ) || ( unit->BreakFailed ) ) && ( ! unit->Recovery[SERIAL_PIPE_OUT].Waiting ) )
	{
		if ( ! unit->WriteActive )
		{
			unit->WriteActive = (PTR)RemHead( &unit->WriteList );
			unit->WriteLength = 0;
		}

		ioreq = unit->WriteActive;

		if ( ! ioreq )
		{
			return;
		}

		if ( ioreq->IOSer.io_Flags & IOSERF_ABORT )
		{
			unit->WriteActive = NULL;

			Serial_Reply( ctx, ioreq );

			continue;
		}

		if ( unit->BreakFailed )
		{
			unit->WriteActive = NULL;
			ioreq->IOSer.io_Error = SerErr_DevBusy;

			Serial_Reply( ctx, ioreq );

			continue;
		}

		if ( ioreq->IOSer.io_Command == SDCMD_BREAK )
		{
			unit->WriteActive = NULL;

			Serial_BreakStart( ctx, unit, ioreq );

			continue;
		}

		if ( unit->Recovery[SERIAL_PIPE_OUT].Failed )
		{
			unit->WriteActive = NULL;
			ioreq->IOSer.io_Error = SerErr_DevBusy;

			Serial_Reply( ctx, ioreq );

			continue;
		}

		if ( ! unit->WriteLength )
		{
			unit->WriteLength = WriteLength( ioreq );
		}

		remaining = unit->WriteLength - ioreq->IOSer.io_Actual;
		usbio = unit->BulkOut->IORequests[0];
		usbio->io_Command = CMD_WRITE;
		usbio->io_Data = (U8 *)ioreq->IOSer.io_Data + ioreq->IOSer.io_Actual;
		usbio->io_Length = ( remaining > SERIAL_TX_CHUNK ) ? SERIAL_TX_CHUNK : remaining;
		usbio->io_UserData = ioreq;
		unit->WritePending = TRUE;

		SendIO( (PTR)usbio );
	}
}

// --
