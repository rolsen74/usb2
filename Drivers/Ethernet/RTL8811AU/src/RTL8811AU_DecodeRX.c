
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Radio.h"

// --

#define RX_DESCRIPTOR_SIZE 24U
#define RX_LENGTH_MASK 0x3fffU
#define RX_CRC_ERROR 0x4000U
#define RX_ICV_ERROR 0x8000U
#define RX_FW_REPORT 0x10
#define BEACON_HEADER_LENGTH 36U

// --

static void
ReceiveManagement( struct EthernetUnit *unit, const U8 *frame, U32 length )
{
	struct IOSana2Req *ioreq;

	if ( ( ! unit->unit_BeaconReceived ) && ( frame[0] == 0x80 ) && ( length >= BEACON_HEADER_LENGTH ) )
	{
		unit->unit_BeaconReceived = TRUE;

		DebugPrintF( "RTL8811AU: received first beacon (%lu bytes) from %02lx:%02lx:%02lx:%02lx:%02lx:%02lx\n", length,
			(U32) frame[16], (U32) frame[17], (U32) frame[18],
			(U32) frame[19], (U32) frame[20], (U32) frame[21] );
	}

	for( ioreq = (PTR) GetHead( & unit->unit_Pending ); ioreq; ioreq = (PTR) GetSucc( (PTR) ioreq ) )
	{
		if ( ioreq->ios2_Req.io_Command == S2_READMGMT )
		{
			break;
		}
	}

	if ( ! ioreq )
	{
		return;
	}

	Remove( (PTR) ioreq );

	if ( ioreq->ios2_DataLength < length )
	{
		ioreq->ios2_DataLength = 0;
		ioreq->ios2_Req.io_Error = IOERR_BADLENGTH;
	}
	else
	{
		CopyMem( (PTR) frame, ioreq->ios2_Data, length );

		ioreq->ios2_DataLength = length;
	}

	ReplyMsg( (PTR) ioreq );
}

// --

void
RTL8811AU_DecodeRX( struct EthernetUnit *unit, U8 *data, U32 length )
{
	U8 *frame;
	U32 status;
	U32 header;
	U32 payload;
	U32 consumed;

	while ( length >= RX_DESCRIPTOR_SIZE )
	{
		status = data[0] | ( (U32) data[1] << 8 );
		payload = status & RX_LENGTH_MASK;
		header = RX_DESCRIPTOR_SIZE + ( ( data[2] & 0x0f ) * 8 ) + ( data[3] & 3 );

		if ( ( ! payload ) || ( header > length ) || ( payload > length - header ) )
		{
			return;
		}

		frame = data + header;

		if ( ( ! ( status & ( RX_CRC_ERROR | RX_ICV_ERROR ) ) ) &&
			( ! ( data[11] & RX_FW_REPORT ) ) &&
			( payload >= 24 ) )
		{
			if ( ! ( frame[0] & 0x0f ) && ! ( frame[1] & 0x40 ) )
			{
				ReceiveManagement( unit, frame, payload );
			}
			else
			{
				RTL8811AU_ReceiveData( unit, frame, payload );
			}
		}

		consumed = ( header + payload + 7 ) & ~7U;

		if ( consumed >= length )
		{
			return;
		}

		data += consumed;
		length -= consumed;
	}
}

// --
