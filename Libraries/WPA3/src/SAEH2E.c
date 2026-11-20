
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

#define SAE_H2E_LABEL1 "SAE Hash to Element u1 P1"
#define SAE_H2E_LABEL2 "SAE Hash to Element u2 P2"

// --

BIGNUM *
SAE_Bignum( struct USB2SAEAuth *auth )
{
	BIGNUM *bn;

	bn = EC( BN_CTX_get, auth->ctx );

	if ( bn )
	{
		EC( BN_set_flags, bn, SAE_BN_CONSTTIME );
	}

	return ( bn );
}

// --

static S32
SelectField( struct USB2SAEAuth *auth, BIGNUM *out, const BIGNUM *first, const BIGNUM *second, U8 select )
{
	U8 left[SAE_SIZE];
	U8 right[SAE_SIZE];
	U8 mask;
	U32 i;
	S32 success;

	success = EC( BN_bn2binpad, first, left, sizeof( left ) ) == sizeof( left ) &&
		EC( BN_bn2binpad, second, right, sizeof( right ) ) == sizeof( right );

	if ( success )
	{
		mask = 0U - select;

		for( i = 0; i < sizeof( left ); i++ )
		{
			left[i] = ( left[i] & mask ) | ( right[i] & ~mask );
		}

		success = EC( BN_bin2bn, left, sizeof( left ), out ) != NULL;
	}

	Clear_Secret( left, sizeof( left ) );

	Clear_Secret( right, sizeof( right ) );

	return ( success );
}

// --

S32
SAE_CurvePolynomial( struct USB2SAEAuth *auth, BIGNUM *out, BIGNUM *tmp, const BIGNUM *x )
{
	return ( FIELD_MUL( out, x, x ) && FIELD_MUL( out, out, x ) &&
		FIELD_MUL( tmp, auth->a, x ) && FIELD_ADD( out, out, tmp ) &&
		FIELD_ADD( out, out, auth->b ) );
}

// --

static S32
MapField( struct USB2SAEAuth *auth, BIGNUM *u, EC_POINT *point )
{
	BIGNUM *tmp;
	BIGNUM *z;
	BIGNUM *za;
	BIGNUM *power;
	BIGNUM *m;
	BIGNUM *zu2;
	BIGNUM *x1;
	BIGNUM *x2;
	BIGNUM *gx1;
	BIGNUM *gx2;
	BIGNUM *root;
	BIGNUM *one;
	BIGNUM *zero;

	U8 bytes[SAE_SIZE];
	U8 inputSign;
	U8 check;
	U8 qr;
	U32 i;
	S32 success;

	EC( BN_CTX_start, auth->ctx );

	tmp = SAE_Bignum( auth );
	z = SAE_Bignum( auth );
	za = SAE_Bignum( auth );
	power = SAE_Bignum( auth );
	m = SAE_Bignum( auth );
	zu2 = SAE_Bignum( auth );
	x1 = SAE_Bignum( auth );
	x2 = SAE_Bignum( auth );
	gx1 = SAE_Bignum( auth );
	gx2 = SAE_Bignum( auth );
	root = SAE_Bignum( auth );
	one = SAE_Bignum( auth );
	zero = SAE_Bignum( auth );

	success = FALSE;

	if ( ( ! zero ) || ( ! EC( BN_set_word, zero, 0 ) ) || ( ! EC( BN_set_word, one, 1 ) ) ||
		( ! EC( BN_sub, z, auth->prime, zero ) ) || ( ! EC( BN_sub_word, z, 10 ) ) ||
		( ! EC( BN_sub, power, auth->prime, zero ) ) || ( ! EC( BN_sub_word, power, 2 ) ) ||
		( ! FIELD_MUL( zu2, u, u ) ) || ( ! FIELD_MUL( zu2, zu2, z ) ) ||
		( ! FIELD_MUL( m, zu2, zu2 ) ) || ( ! FIELD_ADD( m, m, zu2 ) ) ||
		( ! FIELD_EXP( tmp, m, power ) ) || ( ! FIELD_ADD( tmp, tmp, one ) ) ||
		( ! FIELD_EXP( x1, auth->a, power ) ) ||
		( ! EC( BN_sub, root, auth->prime, auth->b ) ) ||
		( ! FIELD_MUL( x1, x1, root ) ) || ( ! FIELD_MUL( x1, x1, tmp ) ) ||
		( ! FIELD_MUL( za, z, auth->a ) ) || ( ! FIELD_EXP( x2, za, power ) ) ||
		( ! FIELD_MUL( x2, x2, auth->b ) ) ||
		( EC( BN_bn2binpad, m, bytes, sizeof( bytes ) ) != sizeof( bytes ) ) )
	{
		goto done;
	}

	check = 0;

	for( i = 0; i < sizeof( bytes ); i++ )
	{
		check |= bytes[i];
	}

	if ( ( ! SelectField( auth, x1, x2, x1, check == 0 ) ) ||
		( ! FIELD_MUL( x2, zu2, x1 ) ) ||
		( ! SAE_CurvePolynomial( auth, gx1, tmp, x1 ) ) ||
		( ! SAE_CurvePolynomial( auth, gx2, tmp, x2 ) ) )
	{
		goto done;
	}

	// For P-256, (p+1)/4 yields a square root for each quadratic residue.
	if ( ( ! EC( BN_sub, power, auth->prime, zero ) ) || ( ! EC( BN_add_word, power, 1 ) ) ||
		( ! EC( BN_rshift, power, power, 2 ) ) ||
		( ! FIELD_EXP( root, gx1, power ) ) || ( ! FIELD_MUL( tmp, root, root ) ) ||
		( ! EC( BN_sub, tmp, tmp, gx1 ) ) ||
		( EC( BN_bn2binpad, tmp, bytes, sizeof( bytes ) ) != sizeof( bytes ) ) )
	{
		goto done;
	}

	check = 0;

	for( i = 0; i < sizeof( bytes ); i++ )
	{
		check |= bytes[i];
	}

	qr = check == 0;

	if ( ( ! SelectField( auth, x1, x1, x2, qr ) ) ||
		( ! SelectField( auth, gx1, gx1, gx2, qr ) ) ||
		( ! FIELD_EXP( root, gx1, power ) ) ||
		( ! EC( BN_sub, tmp, auth->prime, root ) ) ||
		( EC( BN_bn2binpad, u, bytes, sizeof( bytes ) ) != sizeof( bytes ) ) )
	{
		goto done;
	}

	inputSign = bytes[SAE_SIZE - 1] & 1;

	if ( EC( BN_bn2binpad, root, bytes, sizeof( bytes ) ) == sizeof( bytes ) )
	{
		success = SelectField( auth, root, tmp, root, inputSign ^ ( bytes[SAE_SIZE - 1] & 1 ) ) &&
			EC( EC_POINT_set_affine_coordinates, auth->group, point, x1, root, auth->ctx );
	}

done:

	Clear_Secret( bytes, sizeof( bytes ) );

	EC( BN_CTX_end, auth->ctx );

	return ( success );
}

// --

static S32
MapPassword( struct USB2SAEAuth *auth, BIGNUM *u, const U8 *seed, const char *label, EC_POINT *point )
{
	U8 input[SAE_SIZE + sizeof( SAE_H2E_LABEL1 )];
	U8 wide[2 * SAE_SIZE];
	U32 labelLength;
	S32 success;

	labelLength = strlen( label );

	memcpy( input, label, labelLength );

	input[labelLength] = 1;
	success = Crypto_HMAC_SHA256( & auth->crypto, seed, SAE_SIZE, input, labelLength + 1, wide );

	if ( success )
	{
		memcpy( input, wide, SAE_SIZE );

		memcpy( input + SAE_SIZE, label, labelLength );

		input[SAE_SIZE + labelLength] = 2;
		success = Crypto_HMAC_SHA256( & auth->crypto, seed, SAE_SIZE, input,
			SAE_SIZE + labelLength + 1, wide + SAE_SIZE ) &&
			EC( BN_bin2bn, wide, 48, u ) &&
			EC( BN_nnmod, u, u, auth->prime, auth->ctx ) &&
			MapField( auth, u, point );
	}

	Clear_Secret( input, sizeof( input ) );

	Clear_Secret( wide, sizeof( wide ) );

	return ( success );
}

// --

S32
SAE_H2E( struct USB2SAEAuth *auth, const U8 *ssid, U32 ssidLength, const char *password, U32 length,
	const U8 *station, const U8 *ap )
{
	EC_POINT *second;
	BIGNUM *u;
	BIGNUM *range;
	const U8 *firstAddr;
	const U8 *secondAddr;

	static const U8 zeros[SAE_SIZE] = { 0 };
	U8 addresses[12];
	U8 seed[SAE_SIZE];
	S32 success;

	EC( BN_CTX_start, auth->ctx );

	u = SAE_Bignum( auth );
	range = SAE_Bignum( auth );
	second = EC( EC_POINT_new, auth->group );

	firstAddr = ( memcmp( station, ap, 6 ) > 0 ) ? station : ap;
	secondAddr = firstAddr == station ? ap : station;

	memcpy( addresses, firstAddr, 6 );

	memcpy( addresses + 6, secondAddr, 6 );

	success = range && second &&
		Crypto_HMAC_SHA256( & auth->crypto, ssid, ssidLength, (const U8 *) password, length, seed ) &&
		MapPassword( auth, u, seed, SAE_H2E_LABEL1, auth->pwe ) &&
		MapPassword( auth, u, seed, SAE_H2E_LABEL2, second ) &&
		EC( EC_POINT_add, auth->group, auth->pwe, auth->pwe, second, auth->ctx ) &&
		Crypto_HMAC_SHA256( & auth->crypto, zeros, sizeof( zeros ), addresses, sizeof( addresses ), seed ) &&
		EC( BN_bin2bn, seed, sizeof( seed ), u ) &&
		EC( BN_set_word, range, 0 ) && EC( BN_sub, range, auth->order, range ) &&
		EC( BN_sub_word, range, 1 ) && EC( BN_nnmod, u, u, range, auth->ctx ) &&
		EC( BN_add_word, u, 1 ) &&
		EC( EC_POINT_mul, auth->group, auth->pwe, NULL, auth->pwe, u, auth->ctx ) &&
		( EC( EC_POINT_is_at_infinity, auth->group, auth->pwe ) == 0 );

	Clear_Secret( seed, sizeof( seed ) );

	EC( EC_POINT_clear_free, second );

	EC( BN_CTX_end, auth->ctx );

	return ( success );
}

// --
