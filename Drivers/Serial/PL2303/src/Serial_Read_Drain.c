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

	while( unit->RxHead != unit->RxTail )
	{
		ioreq = (PTR) GetHead( & unit->ReadList );

		if ( ! ioreq )
		{
			break;
		}

		data = ioreq->IOSer.io_Data;
		terms = (U8 *) & ioreq->io_TermArray;
		done = FALSE;

		while( ( unit->RxHead != unit->RxTail ) && ( ioreq->IOSer.io_Actual < ioreq->IOSer.io_Length ) )
		{
			value = unit->RxBuffer[unit->RxTail];
			data[ioreq->IOSer.io_Actual] = value;
			ioreq->IOSer.io_Actual++;
			unit->RxTail = ( unit->RxTail + 1 ) % SERIAL_RX_SIZE;

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

		if ( ( ! done ) && ( ioreq->IOSer.io_Actual != ioreq->IOSer.io_Length ) )
		{
			break;
		}

		Remove( (PTR) ioreq );
		Serial_Reply( ctx, ioreq );
	}

	Serial_Read_Start( ctx, unit );
}

// --
