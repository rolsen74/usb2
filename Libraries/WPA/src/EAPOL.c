
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "WPA.h"

// --

#define WPA_DESCRIPTOR 254
#define WPA_VERSION 1
#define WPA_M1 ( WPA_VERSION | WPA2_PAIRWISE | WPA2_ACK )
#define WPA_M3 ( WPA_M1 | WPA2_MIC | WPA2_INSTALL )
#define WPA_G1 ( WPA_VERSION | WPA2_ACK | WPA2_MIC )
#define WPA_KEY_INDEX 0x30
#define KEY_REPLAY 9
#define KEY_NONCE 17
#define KEY_IV 49
#define KEY_RSC 65
#define KEY_MIC 81
#define KEY_DATA_LENGTH 97

// --

static void
Copy_TKIP_Key( U8 *output, const U8 *key )
{
	memcpy( output, key, 16 );

	// The authenticator's TX Michael key is the client's RX Michael key.
	memcpy( output + 16, key + 24, 8 );

	memcpy( output + 24, key + 16, 8 );
}

// --

static void
Decrypt_GTK( const U8 *kek, const U8 *frame, U8 *output )
{
	U8 seed[32];
	U8 state[256];
	U8 x;
	U8 y;
	U8 swap;
	U32 i;

	memcpy( seed, frame + KEY_IV, 16 );

	memcpy( seed + 16, kek, 16 );

	for( i = 0; i < 256; i++ )
	{
		state[i] = i;
	}

	y = 0;

	for( i = 0; i < 256; i++ )
	{
		y += state[i] + seed[i & 31];
		swap = state[i];
		state[i] = state[y];
		state[y] = swap;
	}

	x = 0;
	y = 0;

	// WPA discards the first 256 RC4 bytes before unwrapping the GTK.
	for( i = 0; i < 256 + USB2WPA_TKIP_KEY_SIZE; i++ )
	{
		x++;
		y += state[x];
		swap = state[x];
		state[x] = state[y];
		state[y] = swap;

		if ( i >= 256 )
		{
			output[i - 256] = frame[WPA2_KEY_HEADER + i - 256] ^ state[(U8) ( state[x] + state[y] )];
		}
	}

	Clear_Secret( seed, sizeof( seed ) );

	Clear_Secret( state, sizeof( state ) );
}

// --

static S32
Check_MIC( struct WirelessKeySession *session, const U8 *frame, U32 length )
{
	U8 packet[WPA2_MAX_EAPOL];
	U8 mic[16];
	U8 difference;
	U32 i;
	S32 verified;

	memcpy( packet, frame, length );

	memset( packet + KEY_MIC, 0, sizeof( mic ) );

	verified = Crypto_HMAC_MD5( & session->Crypto, session->PTK, packet, length, mic );
	difference = 0;

	if ( verified )
	{
		for( i = 0; i < sizeof( mic ); i++ )
		{
			difference |= mic[i] ^ frame[KEY_MIC + i];
		}
	}

	Clear_Secret( mic, sizeof( mic ) );

	return ( verified ? ! difference : -1 );
}

// --

static S32
Make_Reply( struct WirelessKeySession *session, const U8 *frame, U16 flags, S32 message2,
	struct USB2WPAReply *reply )
{
	U8 mac[16];

	reply->Length = WPA2_KEY_HEADER;
	reply->Frame[0] = frame[0];
	reply->Frame[1] = 3;
	reply->Frame[4] = WPA_DESCRIPTOR;
	reply->Frame[5] = flags >> 8;
	reply->Frame[6] = flags;
	reply->Frame[7] = frame[7];
	reply->Frame[8] = frame[8];

	memcpy( reply->Frame + KEY_REPLAY, frame + KEY_REPLAY, WPA2_REPLAY_SIZE );

	if ( message2 )
	{
		reply->Length += sizeof( AssociationWPA );
		reply->Frame[KEY_DATA_LENGTH + 1] = sizeof( AssociationWPA );

		memcpy( reply->Frame + KEY_NONCE, session->SNonce, WPA2_NONCE_SIZE );

		memcpy( reply->Frame + WPA2_KEY_HEADER, AssociationWPA, sizeof( AssociationWPA ) );
	}

	reply->Frame[3] = reply->Length - 4;

	if ( ! Crypto_HMAC_MD5( & session->Crypto, session->PTK, reply->Frame, reply->Length, mac ) )
	{
		Clear_Secret( mac, sizeof( mac ) );

		Clear_Secret( reply, sizeof( *reply ) );

		return ( -1 );
	}

	memcpy( reply->Frame + KEY_MIC, mac, sizeof( mac ) );

	Clear_Secret( mac, sizeof( mac ) );

	return ( 1 );
}

// --

static S32
Match_AP_IE( struct WirelessKeySession *session, const U8 *data, U32 length )
{
	U32 size;
	S32 found;

	found = FALSE;

	while( length >= 2 )
	{
		size = data[1] + 2U;

		if ( size > length )
		{
			return ( FALSE );
		}

		if ( ( size == session->RSNLength ) && ( ! memcmp( data, session->RSN, size ) ) )
		{
			found = TRUE;
		}

		data += size;
		length -= size;
	}

	return ( found && ! length );
}

// --

S32
WPA_Process( struct USB2WPAIFace *self __attribute__((unused)), struct USB2WPASession *handle,
	const U8 *frame, U32 length, struct USB2WPAReply *reply )
{
	struct WirelessKeySession *session;

	U8 gtk[USB2WPA_TKIP_KEY_SIZE];
	U32 packet_length;
	U32 data_length;
	U16 flags;
	U16 index;
	S32 replay;
	S32 pairwise;
	S32 result;

	session = (struct WirelessKeySession *) handle;

	memset( reply, 0, sizeof( *reply ) );

	if ( ( length < WPA2_KEY_HEADER ) || ( ! frame[0] ) || ( frame[0] > 3 ) ||
		( frame[1] != 3 ) || ( frame[4] != WPA_DESCRIPTOR ) || ( frame[7] ) ||
		( frame[8] != USB2WPA_TKIP_KEY_SIZE ) )
	{
		return ( 0 );
	}

	packet_length = 4U + ( (U32) frame[2] << 8 ) + frame[3];
	data_length = ( (U32) frame[KEY_DATA_LENGTH] << 8 ) | frame[KEY_DATA_LENGTH + 1];
	flags = ( (U16) frame[5] << 8 ) | frame[6];
	index = ( flags & WPA_KEY_INDEX ) >> 4;

	if ( ( packet_length > length ) || ( packet_length > WPA2_MAX_EAPOL ) ||
		( packet_length != WPA2_KEY_HEADER + data_length ) )
	{
		return ( 0 );
	}

	replay = memcmp( frame + KEY_REPLAY, session->Replay, WPA2_REPLAY_SIZE );

	if ( flags == WPA_M1 )
	{
		if ( ( session->HavePairwise ) || ( session->HaveMessage1 && replay < 0 ) )
		{
			return ( 0 );
		}

		if ( ( ! session->HaveMessage1 ) || ( memcmp( session->ANonce, frame + KEY_NONCE, WPA2_NONCE_SIZE ) ) )
		{
			if ( ( session->HaveMessage1 ) && ( ! replay ) )
			{
				return ( 0 );
			}

			if ( ! Derive_PTK( session, frame + KEY_NONCE, sizeof( session->PTK ) ) )
			{
				return ( -1 );
			}

			memcpy( session->ANonce, frame + KEY_NONCE, WPA2_NONCE_SIZE );
		}

		memcpy( session->Replay, frame + KEY_REPLAY, WPA2_REPLAY_SIZE );

		session->HaveMessage1 = TRUE;

		return ( Make_Reply( session, frame, WPA_VERSION | WPA2_PAIRWISE | WPA2_MIC, TRUE, reply ) );
	}

	pairwise = ( flags & ~WPA2_SECURE ) == WPA_M3;

	if ( ( ! session->HaveMessage1 ) || ( replay < 0 ) )
	{
		return ( 0 );
	}

	if ( pairwise )
	{
		if ( ( session->Authorized ) || ( ! session->HavePairwise && ! replay ) ||
			( memcmp( session->ANonce, frame + KEY_NONCE, WPA2_NONCE_SIZE ) ) ||
			( ! Match_AP_IE( session, frame + WPA2_KEY_HEADER, data_length ) ) )
		{
			return ( 0 );
		}
	}
	else if ( ( ! session->HavePairwise ) ||
		( ( flags & ~( WPA_KEY_INDEX | WPA2_SECURE | WPA2_INSTALL ) ) != WPA_G1 ) ||
		( data_length != USB2WPA_TKIP_KEY_SIZE ) || ( ! session->Authorized && ! replay ) )
	{
		return ( 0 );
	}

	result = Check_MIC( session, frame, packet_length );

	if ( result <= 0 )
	{
		return ( result );
	}

	if ( pairwise )
	{
		reply->Flags = USB2WPA_CLEAR_REPLY;

		if ( ! session->HavePairwise )
		{
			Copy_TKIP_Key( reply->PairwiseKey, session->PTK + 32 );

			reply->Flags |= USB2WPA_INSTALL_PAIRWISE;
			session->HavePairwise = TRUE;

			Clear_Secret( session->PMK, sizeof( session->PMK ) );
		}

		flags = WPA_VERSION | WPA2_PAIRWISE | WPA2_MIC | ( flags & WPA2_SECURE );
	}
	else
	{
		if ( replay > 0 )
		{
			Decrypt_GTK( session->PTK + 16, frame, gtk );

			Copy_TKIP_Key( reply->GroupKey, gtk );

			Clear_Secret( gtk, sizeof( gtk ) );

			memcpy( reply->GroupRSC, frame + KEY_RSC, sizeof( reply->GroupRSC ) );

			reply->Flags = USB2WPA_INSTALL_GROUP;
			reply->GroupIndex = index;
		}

		if ( ! session->Authorized )
		{
			reply->Flags |= USB2WPA_AUTHORIZE;
			session->Authorized = TRUE;
		}

		flags = WPA_VERSION | WPA2_MIC | WPA2_SECURE | ( flags & WPA_KEY_INDEX );
	}

	memcpy( session->Replay, frame + KEY_REPLAY, WPA2_REPLAY_SIZE );

	return ( Make_Reply( session, frame, flags, FALSE, reply ) );
}

// --
