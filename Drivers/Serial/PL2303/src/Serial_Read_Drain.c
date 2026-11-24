/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Serial_Read_Drain( struct SerialContext *ctx, struct SerialUnit *unit )
{
struct IOExtSer *ioreq;

U8 *data;
U8 *terms;
U8 value;
U32 index;
U32 done;

	while( ( ! unit->Stopped ) && ( ! unit->Detached ) )
	{
		if ( ! unit->ReadActive )
		{
			if ( ( unit->RxHead == unit->RxTail ) && ( ! unit->ReadBreaks ) )
			{
				break;
			}

			unit->ReadActive = (PTR) RemHead( &unit->ReadList );
		}

		ioreq = unit->ReadActive;

		if ( ! ioreq )
		{
			break;
		}

		if ( ioreq->IOSer.io_Flags & IOSERF_ABORT )
		{
			unit->ReadActive = NULL;

			Serial_Reply( ctx, ioreq );

			continue;
		}

		if ( unit->ReadBreaks )
		{
			unit->ReadBreaks--;
			unit->ReadActive = NULL;
			ioreq->IOSer.io_Error = SerErr_DetectedBreak;

			Serial_Reply( ctx, ioreq );

			continue;
		}

		data = ioreq->IOSer.io_Data;
		terms = (U8 *) & ioreq->io_TermArray;
		done = FALSE;

		while( ( unit->RxHead != unit->RxTail ) && ( ioreq->IOSer.io_Actual < ioreq->IOSer.io_Length ) &&
			( ! ( ioreq->IOSer.io_Flags & IOSERF_ABORT ) ) )
		{
			value = unit->RxBuffer[unit->RxTail];
			data[ioreq->IOSer.io_Actual] = value;
			ioreq->IOSer.io_Actual++;
			unit->RxTail = ( unit->RxTail + 1 ) % SERIAL_RX_SIZE;

			if ( ( ioreq->IOSer.io_Length == (U32)-1 ) && ( ! value ) )
			{
				done = TRUE;
			}

			if ( ioreq->io_SerFlags & SERF_EOFMODE )
			{
				for( index = 0; index < sizeof( ioreq->io_TermArray ); index++ )
				{
					if ( value == terms[index] )
					{
						done = TRUE;
						break;
					}
				}
			}

			if ( done )
			{
				break;
			}
		}

		if ( ( ! done ) && ( ! ( ioreq->IOSer.io_Flags & IOSERF_ABORT ) ) &&
			( ioreq->IOSer.io_Actual != ioreq->IOSer.io_Length ) )
		{
			break;
		}

		unit->ReadActive = NULL;
		Serial_Reply( ctx, ioreq );
	}

	Serial_Read_Start( ctx, unit );
}

// --
