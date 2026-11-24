
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

static void
AbortOfflineReads( struct EthernetUnit *unit )
{
	struct IOSana2Req *ioreq;
	struct IOSana2Req *next;

	U16 command;

	for( ioreq = (PTR) GetHead( & unit->unit_Pending ); ioreq; ioreq = next )
	{
		next = (PTR) GetSucc( (PTR) ioreq );
		command = ioreq->ios2_Req.io_Command;

		if ( ( ( command == CMD_READ ) || ( command == S2_READORPHAN ) ) &&
			( ! RTL8821CU_DataAllowed( unit, ioreq ) ) )
		{
			Remove( (PTR) ioreq );

			ReportUnitOffline( unit, ioreq );

			ReplyMsg( (PTR) ioreq );
		}
	}
}

// --

void
Cmd_SetKey( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	struct RTL8821CU_Key *key;

	const U8 *sequence;
	U64 number;
	U32 index;
	U32 byte;
	U32 tid;

	index = ioreq->ios2_WireError;
	ioreq->ios2_WireError = 0;

	if ( index >= sizeof( unit->unit_Keys ) / sizeof( unit->unit_Keys[0] ) )
	{
		goto invalid;
	}

	key = & unit->unit_Keys[index];

	if ( ( ioreq->ios2_PacketType == S2ENC_NONE ) && ( ! ioreq->ios2_DataLength ) )
	{
		key->Active = FALSE;

		if ( ! index )
		{
			unit->unit_Operational = FALSE;

			RTL8821CU_SetOnline( unit, FALSE );

			AbortOfflineReads( unit );
		}

		return;
	}

	if ( ( ioreq->ios2_PacketType != S2ENC_CCMP ) || ( ioreq->ios2_DataLength != 16 ) ||
		( ! ioreq->ios2_Data ) || ( ! unit->unit_AssocID ) || ( unit->unit_Encryption != S2ENC_CCMP ) )
	{
		goto invalid;
	}

	sequence = ioreq->ios2_StatData;
	number = 0;

	if ( sequence )
	{
		for( byte = 0; byte < 6; byte++ )
		{
			number |= (U64) sequence[byte] << ( byte * 8 );
		}
	}

	// The first round key is the original key. Reinstallation preserves counters.
	if ( ( ! key->Installed ) || ( memcmp( key->RoundKeys, ioreq->ios2_Data, 16 ) ) )
	{
		Wireless_AESExpand( key->RoundKeys, ioreq->ios2_Data );

		key->TxNumber = 0;
		key->Installed = TRUE;

		for( tid = 0; tid <= WIRELESS_NONQOS; tid++ )
		{
			key->RxNumber[tid] = number;
		}
	}
	else
	{
		for( tid = 0; tid <= WIRELESS_NONQOS; tid++ )
		{
			if ( key->RxNumber[tid] < number )
			{
				key->RxNumber[tid] = number;
			}
		}
	}

	key->Active = TRUE;

	return;

invalid:

	ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
	ioreq->ios2_WireError = S2WERR_INVALID_OPTION;
}

// --
