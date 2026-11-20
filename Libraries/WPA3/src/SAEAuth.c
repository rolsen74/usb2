
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

static S32
MakeCommit( struct USB2SAEAuth *auth )
{
	BIGNUM *mask;
	BIGNUM *scalar;
	BIGNUM *one;
	BIGNUM *x;
	BIGNUM *y;
	EC_POINT *element;

	S32 success;

	EC( BN_CTX_start, auth->ctx );

	mask = SAE_Bignum( auth );
	scalar = SAE_Bignum( auth );
	one = SAE_Bignum( auth );
	x = SAE_Bignum( auth );
	y = SAE_Bignum( auth );
	element = EC( EC_POINT_new, auth->group );

	success = FALSE;

	if ( ( ! y ) || ( ! element ) || ( ! EC( BN_set_word, one, 1 ) ) )
	{
		goto done;
	}

	do
	{
		if ( ( ! EC( BN_priv_rand_range, auth->random, auth->order ) ) ||
			( ! EC( BN_priv_rand_range, mask, auth->order ) ) ||
			( ! EC( BN_mod_add, scalar, auth->random, mask, auth->order, auth->ctx ) ) )
		{
			goto done;
		}
	}
	while( ( EC( BN_cmp, auth->random, one ) <= 0 ) || ( EC( BN_cmp, mask, one ) <= 0 ) ||
		( EC( BN_cmp, scalar, one ) <= 0 ) );

	success = EC( EC_POINT_mul, auth->group, element, NULL, auth->pwe, mask, auth->ctx ) &&
		EC( EC_POINT_invert, auth->group, element, auth->ctx ) &&
		EC( EC_POINT_get_affine_coordinates, auth->group, element, x, y, auth->ctx ) &&
		( EC( BN_bn2binpad, scalar, auth->own, SAE_SIZE ) == SAE_SIZE ) &&
		( EC( BN_bn2binpad, x, auth->own + SAE_SIZE, SAE_SIZE ) == SAE_SIZE ) &&
		( EC( BN_bn2binpad, y, auth->own + 2 * SAE_SIZE, SAE_SIZE ) == SAE_SIZE );

done:

	EC( EC_POINT_clear_free, element );

	EC( BN_CTX_end, auth->ctx );

	return ( success );
}

// --

struct USB2SAEAuth *
WPA3_SAEStart( struct USB2WPA3IFace *self, const U8 *ssid, U32 ssidLength, const char *password,
	U32 length, const U8 *station, const U8 *ap, U8 *commit )
{
	return ( WPA3_SAEStartMode( self, ssid, ssidLength, password, length, station, ap, commit, USB2WPA3_SAE_H2E ) );
}

// --

struct USB2SAEAuth *
WPA3_SAEStartMode( struct USB2WPA3IFace *self, const U8 *ssid, U32 ssidLength, const char *password,
	U32 length, const U8 *station, const U8 *ap, U8 *commit, U32 mode )
{
	struct USB2SAEAuth *auth;

	if ( ( ! ssid ) || ( ! ssidLength ) || ( ssidLength > 32 ) ||
		( ! password ) || ( ! length ) || ( length > 63 ) || ( ! station ) || ( ! ap ) || ( ! commit ) ||
		( ( mode != USB2WPA3_SAE_H2E ) && ( mode != USB2WPA3_SAE_HNP ) ) )
	{
		return ( NULL );
	}

	auth = AllocVecTags( sizeof( *auth ), AVT_ClearWithValue, 0, TAG_END );

	if ( ! auth )
	{
		return ( NULL );
	}

	if ( ! Crypto_Open( & auth->crypto ) )
	{
		goto failed;
	}

	auth->group = EC( EC_GROUP_new_by_curve_name, SAE_OPENSSL_P256 );

	auth->ctx = SAE_CALL( & auth->crypto, BN_CTX_secure_new );

	if ( ( ! auth->group ) || ( ! auth->ctx ) )
	{
		goto failed;
	}

	auth->pwe = EC( EC_POINT_new, auth->group );

	auth->order = EC( EC_GROUP_get0_order, auth->group );

	EC( BN_CTX_start, auth->ctx );

	auth->prime = SAE_Bignum( auth );
	auth->a = SAE_Bignum( auth );
	auth->b = SAE_Bignum( auth );
	auth->random = SAE_Bignum( auth );

	if ( ( ! auth->random ) || ( ! auth->pwe ) || ( ! auth->order ) ||
		( ! EC( EC_GROUP_get_curve, auth->group, auth->prime, auth->a, auth->b, auth->ctx ) ) )
	{
		goto failed;
	}

	if ( mode == USB2WPA3_SAE_H2E )
	{
		if ( ! SAE_H2E( auth, ssid, ssidLength, password, length, station, ap ) )
		{
			goto failed;
		}
	}
	else if ( ! SAE_HuntAndPeck( auth, password, length, station, ap ) )
	{
		goto failed;
	}

	if ( ! MakeCommit( auth ) )
	{
		goto failed;
	}

	commit[0] = SAE_GROUP;
	commit[1] = 0;

	memcpy( commit + 2, auth->own, sizeof( auth->own ) );

	return ( auth );

failed:

	WPA3_SAEDestroy( self, auth );

	return ( NULL );
}

// --

static S32
DeriveKeys( struct USB2SAEAuth *auth, const U8 *peer )
{
	BIGNUM *x;
	BIGNUM *y;
	BIGNUM *scalar;
	BIGNUM *peerScalar;
	BIGNUM *one;
	EC_POINT *element;
	EC_POINT *shared;

	static const U8 zeros[SAE_SIZE] = { 0 };
	static const char label[] = "SAE KCK and PMK";
	U8 input[2 + sizeof( label ) - 1 + SAE_SIZE + 2];
	U8 seed[SAE_SIZE];
	U8 value[SAE_SIZE];
	U32 offset;
	S32 success;

	EC( BN_CTX_start, auth->ctx );

	x = SAE_Bignum( auth );
	y = SAE_Bignum( auth );
	scalar = SAE_Bignum( auth );
	peerScalar = SAE_Bignum( auth );
	one = SAE_Bignum( auth );
	element = EC( EC_POINT_new, auth->group );
	shared = EC( EC_POINT_new, auth->group );

	success = one && element && shared &&
		EC( BN_set_word, one, 1 ) &&
		EC( BN_bin2bn, peer, SAE_SIZE, peerScalar ) &&
		( EC( BN_cmp, peerScalar, one ) > 0 ) &&
		( EC( BN_cmp, peerScalar, auth->order ) < 0 ) &&
		EC( BN_bin2bn, peer + SAE_SIZE, SAE_SIZE, x ) &&
		EC( BN_bin2bn, peer + 2 * SAE_SIZE, SAE_SIZE, y ) &&
		( EC( BN_cmp, x, auth->prime ) < 0 ) && ( EC( BN_cmp, y, auth->prime ) < 0 ) &&
		EC( EC_POINT_set_affine_coordinates, auth->group, element, x, y, auth->ctx ) &&
		( EC( EC_POINT_is_on_curve, auth->group, element, auth->ctx ) == 1 ) &&
		( EC( EC_POINT_is_at_infinity, auth->group, element ) == 0 ) &&
		EC( EC_POINT_mul, auth->group, shared, NULL, auth->pwe, peerScalar, auth->ctx ) &&
		EC( EC_POINT_add, auth->group, shared, shared, element, auth->ctx ) &&
		EC( EC_POINT_mul, auth->group, shared, NULL, shared, auth->random, auth->ctx ) &&
		( EC( EC_POINT_is_at_infinity, auth->group, shared ) == 0 ) &&
		EC( EC_POINT_get_affine_coordinates, auth->group, shared, x, NULL, auth->ctx ) &&
		( EC( BN_bn2binpad, x, value, SAE_SIZE ) == SAE_SIZE ) &&
		Crypto_HMAC_SHA256( & auth->crypto, zeros, sizeof( zeros ), value, sizeof( value ), seed ) &&
		EC( BN_bin2bn, auth->own, SAE_SIZE, scalar ) &&
		EC( BN_mod_add, scalar, scalar, peerScalar, auth->order, auth->ctx ) &&
		( EC( BN_bn2binpad, scalar, value, SAE_SIZE ) == SAE_SIZE );

	if ( success )
	{
		input[0] = 1;
		input[1] = 0;
		offset = 2;

		memcpy( input + offset, label, sizeof( label ) - 1 );

		offset += sizeof( label ) - 1;

		memcpy( input + offset, value, sizeof( value ) );

		offset += sizeof( value );
		input[offset++] = 0;
		input[offset++] = 2; // 512 output bits in little endian.

		success = Crypto_HMAC_SHA256( & auth->crypto, seed, sizeof( seed ), input, offset, auth->kck );
		input[0] = 2;
		success = success && Crypto_HMAC_SHA256( & auth->crypto, seed, sizeof( seed ), input, offset, auth->pmk );

		memcpy( auth->pmkid, value, sizeof( auth->pmkid ) );
	}

	Clear_Secret( input, sizeof( input ) );

	Clear_Secret( value, sizeof( value ) );

	Clear_Secret( seed, sizeof( seed ) );

	EC( EC_POINT_clear_free, shared );

	EC( EC_POINT_clear_free, element );

	EC( BN_CTX_end, auth->ctx );

	return ( success );
}

// --

static S32
ConfirmHash( struct USB2SAEAuth *auth, const U8 *counter, const U8 *first, const U8 *second, U8 *out )
{
	U8 input[2 + 6 * SAE_SIZE];
	S32 success;

	memcpy( input, counter, 2 );

	memcpy( input + 2, first, 3 * SAE_SIZE );

	memcpy( input + 2 + 3 * SAE_SIZE, second, 3 * SAE_SIZE );

	success = Crypto_HMAC_SHA256( & auth->crypto, auth->kck, sizeof( auth->kck ), input, sizeof( input ), out );

	Clear_Secret( input, sizeof( input ) );

	return ( success );
}

// --

S32
WPA3_SAECommit( struct USB2WPA3IFace *self __attribute__((unused)), struct USB2SAEAuth *auth,
	const U8 *commit, U32 length, U8 *confirm )
{
	if ( ( ! auth ) || ( ! commit ) || ( ! confirm ) || ( length != USB2SAE_COMMIT_SIZE ) ||
		( commit[0] != SAE_GROUP ) || ( commit[1] ) ||
		( ! memcmp( commit + 2, auth->own, sizeof( auth->own ) ) ) )
	{
		return ( FALSE );
	}

	if ( auth->committed )
	{
		if ( memcmp( commit + 2, auth->peer, sizeof( auth->peer ) ) )
		{
			return ( FALSE );
		}
	}
	else
	{
		if ( ! DeriveKeys( auth, commit + 2 ) )
		{
			return ( FALSE );
		}

		memcpy( auth->peer, commit + 2, sizeof( auth->peer ) );

		auth->committed = TRUE;
	}

	if ( auth->sendConfirm >= 0xfffe )
	{
		return ( FALSE );
	}

	auth->sendConfirm++;
	confirm[0] = auth->sendConfirm;
	confirm[1] = auth->sendConfirm >> 8;

	return ( ConfirmHash( auth, confirm, auth->own, auth->peer, confirm + 2 ) );
}

// --

S32
WPA3_SAEConfirm( struct USB2WPA3IFace *self __attribute__((unused)), struct USB2SAEAuth *auth,
	const U8 *confirm, U32 length, U8 *pmk, U8 *pmkid )
{
	U8 expected[SAE_SIZE];
	U8 difference;
	U32 i;

	if ( ( ! auth ) || ( ! auth->committed ) || ( ! confirm ) || ( length < USB2SAE_CONFIRM_SIZE ) ||
		( ! pmk ) || ( ! pmkid ) ||
		( ! ConfirmHash( auth, confirm, auth->peer, auth->own, expected ) ) )
	{
		return ( FALSE );
	}

	difference = 0;

	for( i = 0; i < sizeof( expected ); i++ )
	{
		difference |= expected[i] ^ confirm[2 + i];
	}

	Clear_Secret( expected, sizeof( expected ) );

	if ( difference )
	{
		return ( FALSE );
	}

	memcpy( pmk, auth->pmk, sizeof( auth->pmk ) );

	memcpy( pmkid, auth->pmkid, sizeof( auth->pmkid ) );

	return ( TRUE );
}

// --

void
WPA3_SAEDestroy( struct USB2WPA3IFace *self __attribute__((unused)), struct USB2SAEAuth *auth )
{
	if ( auth->pwe )
	{
		EC( EC_POINT_clear_free, auth->pwe );
	}

	if ( auth->ctx )
	{
		EC( BN_CTX_free, auth->ctx );
	}

	if ( auth->group )
	{
		EC( EC_GROUP_free, auth->group );
	}

	Crypto_Close( & auth->crypto );

	Clear_Secret( auth, sizeof( *auth ) );

	FreeVec( auth );
}

// --
