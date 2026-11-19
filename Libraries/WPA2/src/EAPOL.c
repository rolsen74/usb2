
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define M1_FLAGS ( WPA2_DESCRIPTOR_V2 | WPA2_PAIRWISE | WPA2_ACK )
#define M2_FLAGS ( WPA2_DESCRIPTOR_V2 | WPA2_PAIRWISE | WPA2_MIC )
#define M3_FLAGS ( M1_FLAGS | WPA2_INSTALL | WPA2_MIC | WPA2_SECURE | WPA2_ENCRYPTED )
#define M4_FLAGS ( M2_FLAGS | WPA2_SECURE )
#define GROUP1_FLAGS ( WPA2_DESCRIPTOR_V2 | WPA2_ACK | WPA2_MIC | WPA2_SECURE | WPA2_ENCRYPTED )
#define GROUP2_FLAGS ( WPA2_DESCRIPTOR_V2 | WPA2_MIC | WPA2_SECURE )

#define KEY_REPLAY 9
#define KEY_NONCE 17
#define KEY_RSC 65
#define KEY_MIC 81
#define KEY_DATA_LENGTH 97

// --

static S32
Derive_PTK( struct USB2WPA2Session *session, const U8 *anonce )
{
	static const U8 label[] = "Pairwise key expansion";

	U8 input[sizeof( label ) + 6 + 6 + WPA2_NONCE_SIZE * 2 + 1];
	U8 hash[20];
	U32 length;
	U32 offset;
	S32 success;

	if ( ! Crypto_Random( & session->Crypto, session->SNonce, sizeof( session->SNonce ) ) )
	{
		return ( FALSE );
	}

	memcpy( input, label, sizeof( label ) );

	offset = sizeof( label );

	if ( memcmp( session->AP, session->Station, 6 ) < 0 )
	{
		memcpy( input + offset, session->AP, 6 );

		memcpy( input + offset + 6, session->Station, 6 );
	}
	else
	{
		memcpy( input + offset, session->Station, 6 );

		memcpy( input + offset + 6, session->AP, 6 );
	}

	offset += 12;

	if ( memcmp( anonce, session->SNonce, WPA2_NONCE_SIZE ) < 0 )
	{
		memcpy( input + offset, anonce, WPA2_NONCE_SIZE );

		memcpy( input + offset + WPA2_NONCE_SIZE, session->SNonce, WPA2_NONCE_SIZE );
	}
	else
	{
		memcpy( input + offset, session->SNonce, WPA2_NONCE_SIZE );

		memcpy( input + offset + WPA2_NONCE_SIZE, anonce, WPA2_NONCE_SIZE );
	}

	success = TRUE;

	for( offset = 0; offset < sizeof( session->PTK ); offset += sizeof( hash ) )
	{
		input[sizeof( input ) - 1] = offset / sizeof( hash );

		if ( ! Crypto_HMAC( & session->Crypto, session->PMK, sizeof( session->PMK ), input, sizeof( input ), hash ) )
		{
			success = FALSE;
			break;
		}

		length = sizeof( session->PTK ) - offset;
		length = length < sizeof( hash ) ? length : sizeof( hash );

		memcpy( session->PTK + offset, hash, length );
	}

	Clear_Secret( hash, sizeof( hash ) );

	return ( success );
}

// --

static S32
Make_Reply( struct USB2WPA2Session *session, const U8 *request, U16 flags, S32 message2,
	struct USB2WPA2Reply *reply )
{
	U8 hash[20];
	U32 data_size;
	S32 success;

	data_size = message2 ? sizeof( AssociationRSN ) : 0;
	reply->Length = WPA2_KEY_HEADER + data_size;
	reply->Frame[0] = request[0];
	reply->Frame[1] = 3;
	reply->Frame[3] = reply->Length - 4;
	reply->Frame[4] = 2;
	reply->Frame[5] = flags >> 8;
	reply->Frame[6] = flags;
	reply->Frame[KEY_DATA_LENGTH + 1] = data_size;

	memcpy( reply->Frame + KEY_REPLAY, request + KEY_REPLAY, WPA2_REPLAY_SIZE );

	if ( message2 )
	{
		memcpy( reply->Frame + KEY_NONCE, session->SNonce, WPA2_NONCE_SIZE );

		memcpy( reply->Frame + WPA2_KEY_HEADER, AssociationRSN, data_size );
	}

	success = Crypto_HMAC( & session->Crypto, session->PTK, 16, reply->Frame, reply->Length, hash );

	if ( success )
	{
		memcpy( reply->Frame + KEY_MIC, hash, 16 );
	}

	Clear_Secret( hash, sizeof( hash ) );

	return ( success ? 1 : -1 );
}

// --

static S32
Check_MIC( struct USB2WPA2Session *session, const U8 *frame, U32 length )
{
	U8 packet[WPA2_MAX_EAPOL];
	U8 hash[20];
	U8 difference;
	U32 i;
	S32 success;

	memcpy( packet, frame, length );

	memset( packet + KEY_MIC, 0, 16 );

	success = Crypto_HMAC( & session->Crypto, session->PTK, 16, packet, length, hash );
	difference = 0;

	if ( success )
	{
		for( i = 0; i < 16; i++ )
		{
			difference |= hash[i] ^ frame[KEY_MIC + i];
		}
	}

	Clear_Secret( hash, sizeof( hash ) );

	return ( success ? difference == 0 : -1 );
}

// --

static S32
Extract_GTK( struct USB2WPA2Session *session, const U8 *frame, U32 length, S32 pairwise,
	struct USB2WPA2Reply *reply )
{
	static const U8 gtk_type[] = { 0, 0x0f, 0xac, 1 };

	U8 data[WPA2_MAX_EAPOL + 16];
	U32 index;
	U32 size;
	S32 same_rsn;
	S32 success;

	if ( ( length < 24 ) || ( length % 8 ) )
	{
		return ( FALSE );
	}

	success = Crypto_Unwrap( & session->Crypto, session->PTK + 16, frame + WPA2_KEY_HEADER, length, data );

	if ( ! success )
	{
		goto done;
	}

	length -= 8;
	index = 0;
	same_rsn = ! pairwise;

	while( length - index >= 2 )
	{
		size = data[index + 1];

		if ( ( data[index] == 0xdd ) && ( ! size ) )
		{
			break; // AES key wrap padding starts with 0xdd.
		}

		if ( size > length - index - 2 )
		{
			success = FALSE;
			goto done;
		}

		if ( ( data[index] == 48 ) && ( pairwise ) )
		{
			if ( ( same_rsn ) || ( size + 2 != session->RSNLength ) ||
				( memcmp( data + index, session->RSN, size + 2 ) ) )
			{
				success = FALSE;
				goto done;
			}

			same_rsn = TRUE;
		}
		else if ( ( data[index] == 0xdd ) && ( size >= sizeof( gtk_type ) ) &&
			( ! memcmp( data + index + 2, gtk_type, sizeof( gtk_type ) ) ) )
		{
			if ( ( size != 22 ) || ( reply->GroupIndex ) || ( ! ( data[index + 6] & 3 ) ) )
			{
				success = FALSE;
				goto done;
			}

			reply->GroupIndex = data[index + 6] & 3;

			memcpy( reply->GroupKey, data + index + 8, sizeof( reply->GroupKey ) );

			memcpy( reply->GroupRSC, frame + KEY_RSC, sizeof( reply->GroupRSC ) );
		}

		index += 2 + size;
	}

	success = same_rsn && reply->GroupIndex;

done:

	Clear_Secret( data, sizeof( data ) );

	return ( success );
}

// --

S32
WPA2_Process( struct USB2WPA2IFace *self __attribute__((unused)), struct USB2WPA2Session *session,
	const U8 *frame, U32 length, struct USB2WPA2Reply *reply )
{
	U32 body;
	U32 data_length;
	U16 flags;
	S32 pairwise;
	S32 replay;
	S32 result;

	memset( reply, 0, sizeof( *reply ) );

	if ( ( length < WPA2_KEY_HEADER ) || ( ! frame[0] ) || ( frame[0] > 3 ) ||
		( frame[1] != 3 ) || ( frame[4] != 2 ) )
	{
		return ( 0 );
	}

	body = ( (U32) frame[2] << 8 ) | frame[3];
	data_length = ( (U32) frame[KEY_DATA_LENGTH] << 8 ) | frame[KEY_DATA_LENGTH + 1];
	flags = ( (U16) frame[5] << 8 ) | frame[6];

	if ( ( body + 4 > length ) || ( body + 4 > WPA2_MAX_EAPOL ) || ( WPA2_KEY_HEADER + data_length != body + 4 ) )
	{
		return ( 0 );
	}

	replay = memcmp( frame + KEY_REPLAY, session->Replay, WPA2_REPLAY_SIZE );

	if ( flags == M1_FLAGS )
	{
		if ( ( session->Authorized ) || ( session->HaveMessage1 && ( replay < 0 ) ) )
		{
			return ( 0 );
		}

		if ( ( ! session->HaveMessage1 ) || ( memcmp( session->ANonce, frame + KEY_NONCE, WPA2_NONCE_SIZE ) ) )
		{
			if ( ( session->HaveMessage1 && ! replay ) ||
				( ! Derive_PTK( session, frame + KEY_NONCE ) ) )
			{
				return ( session->HaveMessage1 && ! replay ? 0 : -1 );
			}

			memcpy( session->ANonce, frame + KEY_NONCE, WPA2_NONCE_SIZE );
		}

		memcpy( session->Replay, frame + KEY_REPLAY, WPA2_REPLAY_SIZE );

		session->HaveMessage1 = TRUE;

		return ( Make_Reply( session, frame, M2_FLAGS, TRUE, reply ) );
	}

	pairwise = flags == M3_FLAGS;

	if ( ( ! session->HaveMessage1 ) || ( replay < 0 ) ||
		( pairwise && ( ( ! session->Authorized && ! replay ) ||
			memcmp( session->ANonce, frame + KEY_NONCE, WPA2_NONCE_SIZE ) ) ) ||
		( ( ! pairwise ) && ( flags != GROUP1_FLAGS || ! session->Authorized ) ) )
	{
		return ( 0 );
	}

	result = Check_MIC( session, frame, body + 4 );

	if ( result <= 0 )
	{
		return ( result );
	}

	if ( ! Extract_GTK( session, frame, data_length, pairwise, reply ) )
	{
		Clear_Secret( reply, sizeof( *reply ) );

		return ( 0 );
	}

	if ( pairwise )
	{
		reply->Flags = USB2WPA2_CLEAR_REPLY;

		if ( ! session->Authorized )
		{
			reply->Flags |= USB2WPA2_INSTALL_PAIRWISE | USB2WPA2_INSTALL_GROUP | USB2WPA2_AUTHORIZE;

			memcpy( reply->PairwiseKey, session->PTK + 32, sizeof( reply->PairwiseKey ) );

			Clear_Secret( session->PMK, sizeof( session->PMK ) );

			session->Authorized = TRUE;
		}
	}
	else if ( replay > 0 )
	{
		reply->Flags = USB2WPA2_INSTALL_GROUP;
	}

	memcpy( session->Replay, frame + KEY_REPLAY, WPA2_REPLAY_SIZE );

	return ( Make_Reply( session, frame, pairwise ? M4_FLAGS : GROUP2_FLAGS, FALSE, reply ) );
}
