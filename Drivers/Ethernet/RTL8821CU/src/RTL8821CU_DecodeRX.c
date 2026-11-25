
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

#define RX_ERROR_MASK 0xc0
#define RX_FIRMWARE_REPORT 0x10
#define BEACON_HEADER_LENGTH 36U

// --

static void
ReceiveManagement( struct EthernetUnit *unit, const U8 *frame, U32 length )
{
	struct IOSana2Req *ioreq;

	U32 interval;
	U32 ticks;

	if ( ( frame[0] == 0x80 ) && ( length >= BEACON_HEADER_LENGTH ) && ( unit->unit_AssocID ) &&
		( ! memcmp( frame + 10, unit->unit_BSSID, RTL8821CU_ADDR_SIZE ) ) &&
		( ! memcmp( frame + 16, unit->unit_BSSID, RTL8821CU_ADDR_SIZE ) ) )
	{
		interval = frame[32] | ( (U32) frame[33] << 8 );
		ticks = ( interval * 1024 + 1000000 / TICKS_PER_SECOND - 1 ) / ( 1000000 / TICKS_PER_SECOND );
		unit->unit_BeaconAge = 0;
		unit->unit_BeaconLimit = 10 * ticks;

		if ( unit->unit_BeaconLimit < RTL8821CU_BEACON_TIMEOUT )
		{
			unit->unit_BeaconLimit = RTL8821CU_BEACON_TIMEOUT;
		}
	}

	if ( ( ! unit->unit_BeaconReceived ) && ( frame[0] == 0x80 ) && ( length >= BEACON_HEADER_LENGTH ) )
	{
		unit->unit_BeaconReceived = TRUE;

		DebugPrintF( "RTL8821CU: received first beacon (%lu bytes) from %02lx:%02lx:%02lx:%02lx:%02lx:%02lx\n", length,
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
RTL8821CU_DecodeRX( struct EthernetUnit *unit, U8 *buffer, U32 length )
{
	U8 *frame;

	U32 frame_size;
	U32 offset;
	U32 advance;

	while( length >= RTL8821CU_RX_DESC_SIZE )
	{
		frame_size = ( (U32) ( buffer[1] & 0x3f ) << 8 ) | buffer[0];
		offset = RTL8821CU_RX_DESC_SIZE + ( buffer[2] & 0x0f ) * 8 + ( buffer[3] & 3 );

		if ( ( ! frame_size ) || ( offset + frame_size > length ) )
		{
			unit->unit_BadPackets++;

			return;
		}

		if ( ! ( buffer[11] & RX_FIRMWARE_REPORT ) )
		{
			if ( ( buffer[1] & RX_ERROR_MASK ) || ( frame_size < RTL8821CU_WLAN_HEADER ) )
			{
				unit->unit_BadPackets++;
			}
			else
			{
				frame = buffer + offset;

				if ( ( ! ( frame[0] & 0x0f ) ) && ( ! ( frame[1] & 0x40 ) ) )
				{
					ReceiveManagement( unit, frame, frame_size );
				}
				else
				{
					RTL8821CU_ReceiveData( unit, frame, frame_size );
				}
			}
		}

		advance = ( offset + frame_size + 7 ) & ~7U;

		if ( advance >= length )
		{
			return;
		}

		buffer += advance;
		length -= advance;
	}
}

// --
