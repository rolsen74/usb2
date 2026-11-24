
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

void
RTL8188EU_DecodeRX( struct EthernetUnit *unit, U8 *data, U32 length )
{
	U32 frames;
	U32 offset;
	U32 payload;
	U32 header;
	U32 consumed;

	if ( length < RTL8188EU_RX_DESC_SIZE )
	{
		return;
	}

	frames = data[10] ? data[10] : 1;

	for( offset = 0; ( frames ) && ( length - offset >= RTL8188EU_RX_DESC_SIZE ); frames-- )
	{
		payload = data[offset] | ( ( (U32) data[offset + 1] & 0x3f ) << 8 );
		header = RTL8188EU_RX_DESC_SIZE + ( ( data[offset + 2] & 15 ) * 8 ) + ( data[offset + 3] & 3 );

		if ( ( header > length - offset ) || ( ! payload ) || ( payload > length - offset - header ) )
		{
			break;
		}

		if ( ( ! ( data[offset + 1] & 0xc0 ) ) && ( ! ( data[offset + 13] & 0xc0 ) ) &&
			( payload >= 24 ) )
		{
			if ( ( data[offset + header] & 0x0f ) == 0 )
			{
				RTL8188EU_ReceiveManagement( unit, data + offset + header, payload );
			}
			else
			{
				RTL8188EU_ReceiveData( unit, data + offset + header, payload );
			}
		}

		consumed = ( header + payload + 127 ) & ~127U;

		if ( consumed > length - offset )
		{
			break;
		}

		offset += consumed;
	}
}

// --
