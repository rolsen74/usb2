
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Commands.h"
#include "Radio.h"
#include <devices/usb2rtl8188.h>

// --

void
Cmd_SetKey( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	struct RTL8188EUKey *key;

	const U8 *sequence;
	const U8 *data;
	const U8 *previous;
	U64 number;
	U32 index;
	U32 type;
	U32 size;
	U32 i;
	U8 difference;

	index = ioreq->ios2_WireError;
	ioreq->ios2_WireError = 0;

	if ( ioreq->ios2_PacketType == USB2WIRELESSENC_BIP_CMAC )
	{
		if ( ( ! unit->unit_SAE ) || ( ! unit->unit_AssocID ) ||
			( ioreq->ios2_DataLength != RTL8188EU_TEMPORAL_KEY_SIZE ) ||
			( ! ioreq->ios2_Data ) || ( ! ioreq->ios2_StatData ) ||
			( ! Wireless_InstallIGTK( unit->unit_IGTK, index, ioreq->ios2_Data, ioreq->ios2_StatData ) ) )
		{
			goto invalid;
		}

		return;
	}

	if ( ( index >= RTL8188EU_KEY_COUNT ) ||
		( index == USB2RTL8188_TKIP_GROUP_ZERO && unit->unit_Encryption != S2ENC_TKIP ) )
	{
		goto invalid;
	}

	key = & unit->unit_Keys[index];

	if ( ( ioreq->ios2_PacketType == S2ENC_NONE ) && ( ! ioreq->ios2_DataLength ) )
	{
		key->active = FALSE;

		return;
	}

	type = ioreq->ios2_PacketType;

	if ( ( unit->unit_SAE && type != S2ENC_CCMP ) ||
		( index == USB2RTL8188_TKIP_GROUP_ZERO && type != S2ENC_TKIP ) )
	{
		goto invalid;
	}

	if ( ( type == S2ENC_TKIP ) && ( IRTL8188Timer ) &&
		( RTL8188EU_TKIPTime() >= unit->unit_TKIPBlockedUntil ) )
	{
		size = sizeof( key->tkip_key );
		previous = key->tkip_key;
	}
	else if ( type == S2ENC_CCMP )
	{
		size = RTL8188EU_TEMPORAL_KEY_SIZE;
		previous = key->round_keys;
	}
	else
	{
		goto invalid;
	}

	if ( ( ioreq->ios2_DataLength != size ) || ( ! ioreq->ios2_Data ) || ( ! unit->unit_AssocID ) )
	{
		goto invalid;
	}

	data = ioreq->ios2_Data;
	sequence = ioreq->ios2_StatData;
	number = 0;
	difference = 0;

	if ( sequence )
	{
		for( i = 0; i < 6; i++ )
		{
			number |= (U64) sequence[i] << ( i * 8 );
		}
	}

	for( i = 0; i < RTL8188EU_TEMPORAL_KEY_SIZE; i++ )
	{
		difference |= previous[i] ^ data[i];
	}

	// Reinstalling the same key must preserve transmit and receive counters.
	if ( ( key->type != type ) || ( difference ) )
	{
		if ( type == S2ENC_CCMP )
		{
			Wireless_AESExpand( key->round_keys, data );
		}

		key->tx_number = 0;
		key->rx_number[WIRELESS_MGMT] = 0;
		key->type = type;

		for( i = 0; i <= RTL8188EU_NONQOS; i++ )
		{
			key->rx_number[i] = number;
		}
	}
	else
	{
		for( i = 0; i <= RTL8188EU_NONQOS; i++ )
		{
			if ( key->rx_number[i] < number )
			{
				key->rx_number[i] = number;
			}
		}
	}

	if ( type == S2ENC_TKIP )
	{
		memcpy( key->tkip_key, data, size );
	}

	key->active = TRUE;

	if ( unit->unit_Encryption == S2ENC_NONE )
	{
		unit->unit_Encryption = type;
	}

	return;

invalid:

	ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
	ioreq->ios2_WireError = S2WERR_INVALID_OPTION;
}

// --
