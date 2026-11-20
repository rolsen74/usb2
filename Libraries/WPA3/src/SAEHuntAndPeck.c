
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#include "SAE.h"

// --

#define SAE_HUNT_ROUNDS 40
#define SAE_PASSWORD_MAX 63

// --

static U8
FieldBelowPrime( const U8 *bytes, const U8 *prime )
{
	U32 carry;
	U32 i;

	carry = 0;

	for( i = SAE_SIZE; i; i-- )
	{
		carry = ( (U32) bytes[i - 1] - prime[i - 1] - carry ) >> 31;
	}

	return ( carry );
}

// --

S32
SAE_HuntAndPeck( struct USB2SAEAuth *auth, const char *password, U32 length, const U8 *station, const U8 *ap )
{
	BIGNUM *x;
	BIGNUM *y;
	BIGNUM *tmp;
	BIGNUM *blind;
	BIGNUM *range;
	BIGNUM *power;
	BIGNUM *field;
	BIGNUM *zero;
	const U8 *higher;
	const U8 *lower;

	static const char label[] = "SAE Hunting and Pecking";
	U8 input[2 + sizeof( label ) - 1 + SAE_SIZE + 2];
	U8 seed[SAE_SIZE];
	U8 candidate[SAE_SIZE];
	U8 prime[SAE_SIZE];
	U8 xSelected[SAE_SIZE];
	U8 yBytes[SAE_SIZE];
	U8 guess[SAE_PASSWORD_MAX + 1];
	U8 dummy[SAE_PASSWORD_MAX];
	U8 addresses[12];
	U8 selected;
	U8 inputSign;
	U8 choose;
	U8 rangeMask;
	U8 valid;
	U8 different;
	U8 mask;
	U32 count;
	U32 i;
	S32 success;

	EC( BN_CTX_start, auth->ctx );

	x = SAE_Bignum( auth );
	y = SAE_Bignum( auth );
	tmp = SAE_Bignum( auth );
	blind = SAE_Bignum( auth );
	range = SAE_Bignum( auth );
	power = SAE_Bignum( auth );
	field = SAE_Bignum( auth );
	zero = SAE_Bignum( auth );

	higher = memcmp( station, ap, 6 ) > 0 ? station : ap;
	lower = higher == station ? ap : station;
	selected = 0;
	inputSign = 0;
	success = FALSE;

	memset( xSelected, 0, sizeof( xSelected ) );

	memcpy( addresses, higher, 6 );

	memcpy( addresses + 6, lower, 6 );

	input[0] = 1;
	input[1] = 0;

	memcpy( input + 2, label, sizeof( label ) - 1 );

	if ( ( ! zero ) || ( length > sizeof( dummy ) ) ||
		( ! Crypto_Random( & auth->crypto, dummy, length ) ) ||
		( ! EC( BN_set_word, zero, 0 ) ) ||
		( ! EC( BN_sub, power, auth->prime, zero ) ) ||
		( ! EC( BN_add_word, power, 1 ) ) ||
		( ! EC( BN_rshift, power, power, 2 ) ) ||
		( ! EC( BN_sub, range, auth->prime, zero ) ) ||
		( ! EC( BN_sub_word, range, 1 ) ) ||
		( EC( BN_bn2binpad, auth->prime, prime, sizeof( prime ) ) != sizeof( prime ) ) )
	{
		goto done;
	}

	memcpy( input + 2 + sizeof( label ) - 1, prime, sizeof( prime ) );

	input[sizeof( input ) - 2] = 0;
	input[sizeof( input ) - 1] = 1; // 256 output bits in little endian.

	// Always perform the full search. After the first candidate, use a dummy password.
	for( count = 1; count <= SAE_HUNT_ROUNDS; count++ )
	{
		mask = 0U - selected;

		for( i = 0; i < length; i++ )
		{
			guess[i] = ( dummy[i] & mask ) | ( (U8) password[i] & ~mask );
		}

		guess[length] = count;

		if ( ( ! Crypto_HMAC_SHA256( & auth->crypto, addresses, sizeof( addresses ), guess, length + 1, seed ) ) ||
			( ! Crypto_HMAC_SHA256( & auth->crypto, seed, sizeof( seed ), input, sizeof( input ), candidate ) ) )
		{
			goto done;
		}

		valid = FieldBelowPrime( candidate, prime );
		rangeMask = 0U - valid;
		candidate[0] &= rangeMask;

		// Multiply the curve value by a fresh nonzero square before the residue test.
		if ( ( ! EC( BN_bin2bn, candidate, sizeof( candidate ), x ) ) ||
			( ! SAE_CurvePolynomial( auth, field, tmp, x ) ) ||
			( ! EC( BN_priv_rand_range, blind, range ) ) || ( ! EC( BN_add_word, blind, 1 ) ) ||
			( ! FIELD_MUL( blind, blind, blind ) ) || ( ! FIELD_MUL( field, field, blind ) ) ||
			( ! FIELD_EXP( y, field, power ) ) || ( ! FIELD_MUL( tmp, y, y ) ) ||
			( ! EC( BN_sub, tmp, tmp, field ) ) ||
			( EC( BN_bn2binpad, tmp, yBytes, sizeof( yBytes ) ) != sizeof( yBytes ) ) )
		{
			goto done;
		}

		different = 0;

		for( i = 0; i < sizeof( yBytes ); i++ )
		{
			different |= yBytes[i];
		}

		choose = valid & ( different == 0 ) & ( selected ^ 1 );
		mask = 0U - choose;

		for( i = 0; i < sizeof( xSelected ); i++ )
		{
			xSelected[i] = ( xSelected[i] & ~mask ) | ( candidate[i] & mask );
		}

		inputSign = ( inputSign & ~mask ) | ( seed[SAE_SIZE - 1] & 1 & mask );
		selected |= choose;
	}

	if ( ( ! selected ) || ( ! EC( BN_bin2bn, xSelected, sizeof( xSelected ), x ) ) ||
		( ! SAE_CurvePolynomial( auth, field, tmp, x ) ) ||
		( ! FIELD_EXP( y, field, power ) ) ||
		( ! EC( BN_sub, tmp, auth->prime, y ) ) ||
		( EC( BN_bn2binpad, y, yBytes, sizeof( yBytes ) ) != sizeof( yBytes ) ) ||
		( EC( BN_bn2binpad, tmp, candidate, sizeof( candidate ) ) != sizeof( candidate ) ) )
	{
		goto done;
	}

	mask = 0U - ( inputSign ^ ( yBytes[SAE_SIZE - 1] & 1 ) );

	for( i = 0; i < sizeof( yBytes ); i++ )
	{
		yBytes[i] = ( yBytes[i] & ~mask ) | ( candidate[i] & mask );
	}

	success = EC( BN_bin2bn, yBytes, sizeof( yBytes ), y ) &&
		EC( EC_POINT_set_affine_coordinates, auth->group, auth->pwe, x, y, auth->ctx );

done:

	Clear_Secret( dummy, sizeof( dummy ) );

	Clear_Secret( guess, sizeof( guess ) );

	Clear_Secret( seed, sizeof( seed ) );

	Clear_Secret( candidate, sizeof( candidate ) );

	Clear_Secret( xSelected, sizeof( xSelected ) );

	Clear_Secret( yBytes, sizeof( yBytes ) );

	EC( BN_CTX_end, auth->ctx );

	return ( success );
}

// --
