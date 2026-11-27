/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

static void
Audio_RateCommand( struct AudioUnit *unit, struct IOStdReq *ioreq )
{
	U32 rate;
	U32 previous;

	if ( ( ioreq->io_Command == AUDIO_CMD_SET_RATE ) && ( ( unit->ReadActive ) || ( unit->WriteCount ) ) )
	{
		ioreq->io_Error = IOERR_UNITBUSY;

		return;
	}

	if ( ( ! ioreq->io_Data ) || ( ioreq->io_Length != sizeof( rate ) ) )
	{
		ioreq->io_Error = IOERR_BADLENGTH;

		return;
	}

	CopyMem( ioreq->io_Data, & rate, sizeof( rate ) );

	if ( ! Audio_Rate_Valid( unit, rate ) )
	{
		ioreq->io_Error = IOERR_NOCMD;

		return;
	}

	if ( ioreq->io_Command == AUDIO_CMD_CHECK_RATE )
	{
		ioreq->io_Actual = sizeof( rate );

		return;
	}

	previous = unit->Format.SampleRate;

	if ( ( rate != previous ) || ( ! unit->RateValid ) )
	{
		Audio_Read_Stop( unit );

		unit->WritePhase = 0;

		if ( ! Audio_Frequency( unit, rate ) )
		{
			unit->RateValid = Audio_Frequency( unit, previous );
			ioreq->io_Error = IOERR_UNITBUSY;

			return;
		}

		unit->Format.SampleRate = rate;
		unit->RateValid = TRUE;
	}

	ioreq->io_Actual = sizeof( rate );
}

// --

SEC_CODE void
Audio_Begin( struct AudioUnit *unit )
{
	struct IOStdReq *ioreq;
	struct AudioWrite *write;

	S32 reset;
	U32 index;
	U32 restore;

	ObtainSemaphore( &unit->Base->Semaphore );

	restore = unit->ReconnectRate;
	unit->ReconnectRate = 0;

	ReleaseSemaphore( &unit->Base->Semaphore );

	if ( ( restore ) && ( restore != unit->Format.SampleRate ) &&
		( Audio_Rate_Valid( unit, restore ) ) && ( Audio_Frequency( unit, restore ) ) )
	{
		unit->Format.SampleRate = restore;
		unit->RatePending = FALSE;
		unit->RateValid = TRUE;
	}

	while(( ioreq = (PTR) GetMsg( unit->BeginPort ) ))
	{
		ObtainSemaphore( & unit->Base->Semaphore );

		reset = unit->ResetStream;
		unit->ResetStream = FALSE;

		ReleaseSemaphore( & unit->Base->Semaphore );

		if ( reset )
		{
			Audio_Read_Stop( unit );

			Audio_Read_Reply( unit, IOERR_ABORTED );

			Audio_Write_Stop( unit, IOERR_ABORTED );
		}

		if ( ioreq->io_Flags & AUDIO_IO_ABORT )
		{
			ioreq->io_Error = IOERR_ABORTED;
		}
		else if ( ioreq->io_Command == AUDIO_CMD_QUERY )
		{
			if ( ! unit->RateValid )
			{
				ioreq->io_Error = IOERR_UNITBUSY;
			}
			else if ( ( ! ioreq->io_Data ) || ( ioreq->io_Length < sizeof( struct AudioFormat ) ) )
			{
				ioreq->io_Error = IOERR_BADLENGTH;
			}
			else
			{
				CopyMem( & unit->Format, ioreq->io_Data, sizeof( struct AudioFormat ) );
				ioreq->io_Actual = sizeof( struct AudioFormat );
			}
		}
		else if ( ( ioreq->io_Command == AUDIO_CMD_SET_RATE ) || ( ioreq->io_Command == AUDIO_CMD_CHECK_RATE ) )
		{
			Audio_RateCommand( unit, ioreq );
		}
		else if ( ( ioreq->io_Command == AUDIO_CMD_GET_GAIN ) || ( ioreq->io_Command == AUDIO_CMD_SET_GAIN ) )
		{
			Audio_GainCommand( unit, ioreq );
		}
		else if ( ( unit->EndPoint & EPADR_Dir_Mask ) &&
			( ( ioreq->io_Command == CMD_READ ) || ( ioreq->io_Command == AUDIO_CMD_READ_TIMED ) ) )
		{
			if ( ( unit->ReadActive ) || ( ! unit->RateValid ) )
			{
				ioreq->io_Error = IOERR_UNITBUSY;
			}
			else if ( ( ! ioreq->io_Data ) || ( ! ioreq->io_Length ) ||
				( ioreq->io_Length % ( unit->Format.Channels * 2 ) ) )
			{
				ioreq->io_Error = IOERR_BADLENGTH;
			}
			else if ( ( unit->RatePending ) &&
				( ! Audio_Frequency( unit, unit->Format.SampleRate ) ) )
			{
				ioreq->io_Error = AUDIO_ERR_RATE_SETUP;
			}
			else
			{
				unit->ReadActive = ioreq;
				unit->DataUsed = 0;
				unit->ReadDiscontinuity = FALSE;
				unit->RatePending = FALSE;

				if ( ioreq->io_Command == AUDIO_CMD_READ_TIMED )
				{
					ioreq->io_Offset = 0;
				}
				else
				{
					unit->EmptyBatches = 0;
					unit->EmptyFailedPackets = 0;
				}

				Audio_Read_Buffered( unit );

				Audio_Read_Start( unit );

				continue;
			}
		}
		else if ( ( ! ( unit->EndPoint & EPADR_Dir_Mask ) ) && ( ioreq->io_Command == CMD_RESET ) )
		{
			if ( unit->WriteCount )
			{
				ioreq->io_Error = IOERR_UNITBUSY;
			}
			else
			{
				unit->WritePhase = 0;
			}
		}
		else if ( ( ! ( unit->EndPoint & EPADR_Dir_Mask ) ) &&
			( ( ioreq->io_Command == CMD_WRITE ) || ( ioreq->io_Command == AUDIO_CMD_QUEUE_WRITE ) ) )
		{
			if ( ( ! unit->RateValid ) || ( unit->WriteCount == AUDIO_WRITE_REQUESTS ) ||
				( ( unit->WriteCount ) && ( ioreq->io_Command == CMD_WRITE ) ) )
			{
				ioreq->io_Error = IOERR_UNITBUSY;
			}
			else if ( ( ! ioreq->io_Data ) || ( ! ioreq->io_Length ) ||
				( ioreq->io_Length % ( unit->Format.Channels * 2 ) ) )
			{
				ioreq->io_Error = IOERR_BADLENGTH;
			}
			else
			{
				index = ( unit->WriteHead + unit->WriteCount ) % AUDIO_WRITE_REQUESTS;
				write = & unit->Writes[index];
				write->Request = ioreq;
				write->Queued = 0;
				write->Done = 0;
				write->Actual = 0;
				write->GapUS = 0;
				unit->WriteCount++;

				Audio_Write_Start( unit );

				continue;
			}
		}
		else
		{
			ioreq->io_Error = IOERR_NOCMD;
		}

		ObtainSemaphore( & unit->Base->Semaphore );

		if ( ioreq->io_Flags & AUDIO_IO_ABORT )
		{
			ioreq->io_Error = IOERR_ABORTED;
		}

		ReplyMsg( (PTR) ioreq );

		ReleaseSemaphore( & unit->Base->Semaphore );
	}
}

// --
