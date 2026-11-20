
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define IEEE80211_DATA			0x08
#define IEEE80211_QOS_DATA		0x88
#define IEEE80211_FROM_DS		0x02
#define IEEE80211_DATA_FLAGS		0xc7
#define IEEE80211_DATA_HEADER		24
#define IEEE80211_QOS_SIZE		2
#define IEEE80211_DST			4
#define IEEE80211_TRANSMITTER		10
#define IEEE80211_SRC			16
#define EAPOL_HEADER_SIZE		4

// --

void
RT2870_CacheEAPOL( struct EthernetUnit *unit, const U8 *frame, U32 length, U32 padding )
{
	static const U8 snap[] = { 0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00, 0x88, 0x8e };
	static const U8 empty[6] = { 0 };
	const U8 *body;
	U32 header;

	if ( ( length < IEEE80211_DATA_HEADER ) ||
		( ( frame[0] != IEEE80211_DATA ) && ( frame[0] != IEEE80211_QOS_DATA ) ) ||
		( ( frame[1] & IEEE80211_DATA_FLAGS ) != IEEE80211_FROM_DS ) ||
		( frame[22] & 0x0f ) ||
		( ! memcmp( unit->unit_Peer, empty, sizeof( empty ) ) ) ||
		( memcmp( frame + IEEE80211_DST, unit->unit_StationAddress, sizeof( unit->unit_StationAddress ) ) ) ||
		( memcmp( frame + IEEE80211_TRANSMITTER, unit->unit_Peer, sizeof( unit->unit_Peer ) ) ) ||
		( memcmp( frame + IEEE80211_SRC, unit->unit_Peer, sizeof( unit->unit_Peer ) ) ) )
	{
		return;
	}

	header = IEEE80211_DATA_HEADER + ( ( frame[0] == IEEE80211_QOS_DATA ) ? IEEE80211_QOS_SIZE : 0 );

	if ( length < header + sizeof( snap ) + EAPOL_HEADER_SIZE )
	{
		return;
	}

	body = frame + header + padding;

	if ( memcmp( body, snap, sizeof( snap ) ) )
	{
		return;
	}

	body += sizeof( snap );
	length -= header + sizeof( snap );

	// An authorized port only accepts clear retries of the initial message 3.
	if ( ( unit->unit_Operational ) && ( ! unit->unit_TKIP ) &&
		( ( length < 7 ) || ( body[1] != 3 ) || ( body[4] != 2 ) ||
			( body[5] != 0x13 ) || ( body[6] != ( unit->unit_SAE ? 0xc8 : 0xca ) ) ) )
	{
		return;
	}

	if ( ( unit->unit_Operational ) && ( unit->unit_TKIP ) &&
		( ( length < 7 ) || ( body[1] != 3 ) || ( body[4] != 254 ) ||
			( ( body[5] & ~2 ) != 1 ) || ( ( body[6] & ~0x70 ) != 0x81 ) ) )
	{
		return;
	}

	RT2870_CacheEAPOLBody( unit, body, length );
}

// --

void
RT2870_CacheEAPOLBody( struct EthernetUnit *unit, const U8 *body, U32 length )
{
	U32 size;

	if ( length < EAPOL_HEADER_SIZE )
	{
		return;
	}

	size = EAPOL_HEADER_SIZE + ( (U32) body[2] << 8 ) + body[3];

	if ( ( size > RT2870_EAPOL_MAX ) || ( size > length ) )
	{
		return;
	}

	if ( ( unit->unit_TKIP ) && ( unit->unit_M4Sent ) && ( ! unit->unit_Operational ) &&
		( unit->unit_TKIPEAPOLReports < 4 ) && ( size >= 7 ) )
	{
		unit->unit_TKIPEAPOLReports++;

		USBINFO( "RT2870: WPA EAPOL RX after message 4, descriptor %lu, key info $%02lx%02lx, length %lu",
			(U32) body[4], (U32) body[5], (U32) body[6], size );
	}

	memcpy( unit->unit_EAPOL, body, size );

	unit->unit_EAPOLLength = size;

	RT2870_DeliverEAPOL( unit );
}

// --
