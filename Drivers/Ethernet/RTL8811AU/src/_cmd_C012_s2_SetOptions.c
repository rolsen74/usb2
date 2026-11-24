
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Radio.h"
#include "Commands.h"

// --

void
Cmd_SetOptions( struct EthernetUnit *unit, const struct RTL8811AU_Board *board, struct IOSana2Req *ioreq,
	struct USB2_EPResource *rx, struct USB2_IORequest **active )
{
	static const U8 empty[6] = { 0 };

	struct TagItem *tags;
	struct TagItem *tag;

	U8 bssid[6];
	U32 channel;
	U32 aid;
	U32 encryption;
	S32 operational;
	S32 changed;
	S32 aid_set;
	S32 recover;

	tags = ioreq->ios2_Data;
	channel = unit->unit_Channel;
	aid = unit->unit_AssocID;
	encryption = unit->unit_Encryption;
	operational = -1;
	aid_set = FALSE;
	recover = ! unit->unit_RadioReady;

	if ( ! tags )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_NULL_POINTER;

		return;
	}

	memcpy( bssid, unit->unit_BSSID, sizeof( bssid ) );

	while( ( tag = NextTagItem( & tags ) ) )
	{
		switch( tag->ti_Tag )
		{
			case S2INFO_Channel:
				if ( ( tag->ti_Data < 1 ) || ( tag->ti_Data > 11 ) )
				{
					goto invalid;
				}

				channel = tag->ti_Data;

				break;

			case S2INFO_BSSID:
				if ( ! tag->ti_Data )
				{
					goto invalid;
				}

				memcpy( bssid, (PTR) tag->ti_Data, sizeof( bssid ) );

				if ( bssid[0] & 1 )
				{
					goto invalid;
				}

				break;

			case S2INFO_AssocID:
				if ( tag->ti_Data > 2007 )
				{
					goto invalid;
				}

				aid = tag->ti_Data;
				aid_set = TRUE;

				break;

			case S2INFO_Encryption:
				if ( ( tag->ti_Data != S2ENC_NONE ) && ( tag->ti_Data != S2ENC_CCMP ) )
				{
					goto invalid;
				}

				encryption = tag->ti_Data;

				break;

			case S2INFO_OperState:
				operational = tag->ti_Data != 0;

				break;

			case S2INFO_PortType:
				if ( tag->ti_Data != S2PORT_MANAGED )
				{
					goto invalid;
				}

				break;

			case S2INFO_Band:
				if ( tag->ti_Data != 1 )
				{
					goto invalid;
				}

				break;

			case S2INFO_SSID:
			case S2INFO_AuthTypes:
			case S2INFO_BeaconInterval:
				break;

			default:
				goto invalid;
		}
	}

	changed = channel != unit->unit_Channel || memcmp( bssid, unit->unit_BSSID, sizeof( bssid ) );

	if ( ( changed ) && ( ! aid_set ) )
	{
		aid = 0;
	}

	changed |= ( aid != unit->unit_AssocID ) || ( recover );

	if ( ( ( aid ) && ( ! memcmp( bssid, empty, sizeof( bssid ) ) ) ) ||
		( ( ! changed ) && ( aid ) && ( encryption != unit->unit_Encryption ) ) ||
		( ( operational == TRUE ) && ( ( ! aid ) ||
			( ( encryption == S2ENC_CCMP ) && ( ( changed ) || ( ! unit->unit_Keys[0].Active ) ) ) ) ) )
	{
		goto invalid;
	}

	if ( ! rx )
	{
		ReportUnitOffline( unit, ioreq );

		return;
	}

	if ( changed )
	{
		if ( *active )
		{
			RTL8811AU_StopRX( *active );

			*active = NULL;
		}

		RTL8811AU_SetOnline( unit, FALSE );

		unit->unit_Operational = FALSE;

		memset( unit->unit_Keys, 0, sizeof( unit->unit_Keys ) );

		if ( ( ( ( recover ) || ( channel != unit->unit_Channel ) ) && ( ! RTL8811AU_TuneRadio( unit, board, channel ) ) ) ||
			( ! RTL8811AU_SetAssociation( unit, bssid, aid ) ) )
		{
			unit->unit_RadioReady = FALSE;
			unit->unit_TxReady = FALSE;
			ioreq->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
			ioreq->ios2_WireError = S2WERR_RCVREL_HDW_ERR;

			Sana2_SignalEvent( & unit->unit_Pending, S2EVENT_ERROR | S2EVENT_HARDWARE );

			Sana2_AbortPending( & unit->unit_Pending, NULL, NULL, IOERR_ABORTED, 0 );

			return;
		}

		unit->unit_RadioReady = TRUE;

		if ( recover )
		{
			DebugPrintF( "RTL8811AU: radio recovered on channel %lu\n", channel );
		}
	}

	unit->unit_Encryption = encryption;

	if ( operational >= 0 )
	{
		unit->unit_RequireOperState = TRUE;
		unit->unit_Operational = operational;
	}

	if ( ( changed ) || ( operational == FALSE ) )
	{
		Sana2_AbortPackets( & unit->unit_Pending, S2ERR_OUTOFSERVICE, S2WERR_UNIT_OFFLINE );
	}

	if ( aid )
	{
		unit->unit_ManualOffline = FALSE;
	}

	RTL8811AU_SetOnline( unit, ( unit->unit_Configured ) && ( unit->unit_TxReady ) && ( aid ) &&
		( encryption == S2ENC_NONE ? ( ! unit->unit_RequireOperState || unit->unit_Operational ) :
			( unit->unit_Operational && unit->unit_Keys[0].Active ) ) );

	return;

invalid:

	ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
	ioreq->ios2_WireError = S2WERR_INVALID_OPTION;
}

// --
