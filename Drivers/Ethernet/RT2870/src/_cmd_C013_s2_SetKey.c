
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Commands.h"

// --

#define RT2870_KEY_LENGTH 16

// --

void
Cmd_SetKey( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	struct RT2870Key *slot;

	const U8 *slot_key;
	const U8 *rsc;
	U32 index;
	U32 i;
	U32 candidate;
	U32 key_size;
	U8 difference;
	U64 number;
	S32 error;

	index = ioreq->ios2_WireError;
	ioreq->ios2_WireError = 0;
	rsc = ioreq->ios2_StatData;

	if ( ( ! unit->unit_AssocID ) || ( ! unit->unit_BeaconResource ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
		ioreq->ios2_WireError = S2WERR_UNIT_OFFLINE;

		return;
	}

	if ( ioreq->ios2_PacketType == USB2RT2870ENC_BIP_CMAC )
	{
		if ( ( ! unit->unit_SAE ) || ( index < 4 ) || ( index > 5 ) ||
			( ioreq->ios2_DataLength != RT2870_KEY_LENGTH ) || ( ! ioreq->ios2_Data ) ||
			( ! rsc ) || ( ! Wireless_InstallIGTK( unit->unit_IGTK, index, ioreq->ios2_Data, rsc ) ) )
		{
			ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
			ioreq->ios2_WireError = S2WERR_INVALID_OPTION;
		}

		return;
	}

	key_size = unit->unit_TKIP ? WIRELESS_TKIP_KEY_SIZE : RT2870_KEY_LENGTH;

	if ( ( index >= ( unit->unit_TKIP ? RT2870_KEY_COUNT : 4 ) ) ||
		( ioreq->ios2_PacketType != ( unit->unit_TKIP ? S2ENC_TKIP : S2ENC_CCMP ) ) ||
		( ioreq->ios2_DataLength != key_size ) || ( ! ioreq->ios2_Data ) ||
		( index && ! rsc ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_INVALID_OPTION;

		return;
	}

	slot = & unit->unit_Keys[index];

	for( candidate = 0; candidate < RT2870_KEY_COUNT; candidate++ )
	{
		if ( unit->unit_ActiveKeys & ( 1U << candidate ) )
		{
			difference = 0;
			slot_key = unit->unit_TKIP ? unit->unit_Keys[candidate].tkip_key : unit->unit_Keys[candidate].round_keys;

			for( i = 0; i < RT2870_KEY_LENGTH; i++ )
			{
				difference |= slot_key[i] ^ ( (const U8 *) ioreq->ios2_Data )[i];
			}

			if ( ! difference )
			{
				// Repeated keys must not establish a new replay counter.
				if ( candidate != index )
				{
					ioreq->ios2_Req.io_Error = S2ERR_BAD_STATE;
					ioreq->ios2_WireError = S2WERR_INVALID_OPTION;
				}

				return;
			}
		}
	}

	if ( ( ! index ) && ( unit->unit_ActiveKeys & RT2870_PAIRWISE_KEY ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_STATE;
		ioreq->ios2_WireError = S2WERR_INVALID_OPTION;

		return;
	}

	number = 0;

	if ( index )
	{
		for( i = 0; i < 6; i++ )
		{
			number |= (U64) rsc[i] << ( 8 * i );
		}
	}

	if ( unit->unit_TKIP )
	{
		if ( ! index )
		{
			// The AP can send a protected group-key message immediately after message 4.
			error = RT2870_SetStationPeer( unit, TRUE );

			if ( error )
			{
				USBERROR( "RT2870: could not program WPA peer, USB error %ld", error );

				RT2870_DropLink( unit, error == USB2Err_Device_Detached );

				ioreq->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
				ioreq->ios2_WireError = S2WERR_RCVREL_HDW_ERR;

				return;
			}
		}

		memcpy( slot->tkip_key, ioreq->ios2_Data, key_size );
	}
	else
	{
		Wireless_AESExpand( slot->round_keys, ioreq->ios2_Data );
	}

	for( i = 0; i <= WIRELESS_NONQOS; i++ )
	{
		slot->rx_number[i] = number;
	}

	unit->unit_ActiveKeys |= 1U << index;

	USBINFO( "RT2870: installed %s key slot %lu", unit->unit_TKIP ? "TKIP" : "CCMP", index );
}

// --
