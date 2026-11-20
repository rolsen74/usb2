
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

S32
Derive_PTK( struct WirelessKeySession *session, const U8 *anonce, U32 ptk_length )
{
	static const U8 label[] = "Pairwise key expansion";

	U8 input[2 + sizeof( label ) - 1 + 6 + 6 + WPA2_NONCE_SIZE * 2 + 2];
	U8 hash[32];
	U32 length;
	U32 size;
	U32 hashSize;
	U32 counter;
	U32 offset;
	S32 success;

	if ( ! Crypto_Random( & session->Crypto, session->SNonce, sizeof( session->SNonce ) ) )
	{
		return ( FALSE );
	}

	offset = session->SAE ? 2 : 0;
	length = session->SAE ? sizeof( label ) - 1 : sizeof( label );

	memcpy( input + offset, label, length );

	offset += length;

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

	size = offset + 2 * WPA2_NONCE_SIZE;
	counter = 0;
	success = TRUE;
	hashSize = session->SAE ? 32 : 20;

	if ( session->SAE )
	{
		input[size++] = ( ptk_length * 8 ) & 0xff;
		input[size++] = ( ptk_length * 8 ) >> 8;
	}
	else
	{
		size++;
	}

	for( offset = 0; offset < ptk_length; offset += hashSize )
	{
		if ( session->SAE )
		{
			counter++;
			input[0] = counter;
			input[1] = counter >> 8;

			success = Crypto_HMAC_SHA256( & session->Crypto, session->PMK,
				sizeof( session->PMK ), input, size, hash );
		}
		else
		{
			input[size - 1] = counter++;

			success = Crypto_HMAC( & session->Crypto, session->PMK,
				sizeof( session->PMK ), input, size, hash );
		}

		if ( ! success )
		{
			break;
		}

		length = ptk_length - offset;
		length = length < hashSize ? length : hashSize;

		memcpy( session->PTK + offset, hash, length );
	}

	Clear_Secret( hash, sizeof( hash ) );

	return ( success );
}

// --
