
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define RX_FCS_SIZE 4
#define RX_ERROR_MASK 0xc0
#define RX_ENCRYPTION_MASK 0x70
#define RX_FIRMWARE_REPORT 0x10

// --

void
RTL8821CU_DecodeRX( struct EthernetUnit *unit, const U8 *buffer, U32 length )
{
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
			if ( ( buffer[1] & RX_ERROR_MASK ) || ( frame_size <= RX_FCS_SIZE ) )
			{
				unit->unit_BadPackets++;
			}
			else if ( ! ( buffer[2] & RX_ENCRYPTION_MASK ) )
			{
				RTL8821CU_ReceiveData( unit, buffer + offset, frame_size - RX_FCS_SIZE );
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
