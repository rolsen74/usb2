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

	if ( ( ioreq->io_Command == AUDIO_CMD_SET_RATE ) && ( unit->ReadActive ) )
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
		unit->BufferedBytes = 0;
		unit->BufferOffset = 0;
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

	while(( ioreq = (PTR) GetMsg( unit->BeginPort ) ))
	{
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
		else if ( ioreq->io_Command == ( ( unit->EndPoint & EPADR_Dir_Mask ) ? CMD_READ : CMD_WRITE ) )
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
			else if ( ( ioreq->io_Command == CMD_READ ) && ( unit->RatePending ) &&
				( ! Audio_Frequency( unit, unit->Format.SampleRate ) ) )
			{
				ioreq->io_Error = AUDIO_ERR_RATE_SETUP;
			}
			else
			{
				unit->ReadActive = ioreq;
				unit->DataUsed = 0;
				unit->WriteQueued = 0;
				unit->WriteLostBytes = 0;
				unit->EmptyBatches = 0;
				unit->EmptyFailedPackets = 0;

				if ( ioreq->io_Command == CMD_READ )
				{
					unit->RatePending = FALSE;

					Audio_Read_Buffered( unit );

					Audio_Read_Start( unit );
				}
				else
				{
					Audio_Write_Start( unit );
				}

				continue;
			}
		}
		else
		{
			ioreq->io_Error = IOERR_NOCMD;
		}

		ReplyMsg( (PTR) ioreq );
	}
}

// --
