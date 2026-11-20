
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#ifndef USB2_SAE_H
#define USB2_SAE_H

#include "SAECrypto.h"

#define SAE_SIZE 32
#define SAE_GROUP 19
#define SAE_OPENSSL_P256 415
#define SAE_BN_CONSTTIME 4

struct USB2SAEAuth
{
	struct Crypto crypto;

	BN_CTX *ctx;
	EC_GROUP *group;
	const BIGNUM *order;
	BIGNUM *prime;
	BIGNUM *a;
	BIGNUM *b;
	BIGNUM *random;
	EC_POINT *pwe;
	U8 own[USB2SAE_COMMIT_SIZE - 2];
	U8 peer[USB2SAE_COMMIT_SIZE - 2];
	U8 kck[SAE_SIZE];
	U8 pmk[SAE_SIZE];
	U8 pmkid[16];
	U16 sendConfirm;
	S32 committed;
};

#define EC( name, ... ) SAE_CALL( & auth->crypto, name, __VA_ARGS__ )
#define FIELD_MUL( r, a, b ) EC( BN_mod_mul, r, a, b, auth->prime, auth->ctx )
#define FIELD_ADD( r, a, b ) EC( BN_mod_add, r, a, b, auth->prime, auth->ctx )
#define FIELD_EXP( r, a, b ) EC( BN_mod_exp_mont_consttime, r, a, b, auth->prime, auth->ctx, NULL )

BIGNUM *SAE_Bignum( struct USB2SAEAuth *auth );
S32 SAE_CurvePolynomial( struct USB2SAEAuth *auth, BIGNUM *out, BIGNUM *tmp, const BIGNUM *x );
S32 SAE_HuntAndPeck( struct USB2SAEAuth *auth, const char *password, U32 length, const U8 *station, const U8 *ap );
S32 SAE_H2E( struct USB2SAEAuth *auth, const U8 *ssid, U32 ssidLength, const char *password, U32 length,
	const U8 *station, const U8 *ap );

struct USB2SAEAuth * APICALL WPA3_SAEStart( struct USB2WPA3IFace *self, const U8 *ssid, U32 ssidLength,
	const char *password, U32 length, const U8 *station, const U8 *ap, U8 *commit );
struct USB2SAEAuth * APICALL WPA3_SAEStartMode( struct USB2WPA3IFace *self, const U8 *ssid, U32 ssidLength,
	const char *password, U32 length, const U8 *station, const U8 *ap, U8 *commit, U32 mode );
S32 APICALL WPA3_SAECommit( struct USB2WPA3IFace *self, struct USB2SAEAuth *auth,
	const U8 *commit, U32 length, U8 *confirm );
S32 APICALL WPA3_SAEConfirm( struct USB2WPA3IFace *self, struct USB2SAEAuth *auth,
	const U8 *confirm, U32 length, U8 *pmk, U8 *pmkid );
void APICALL WPA3_SAEDestroy( struct USB2WPA3IFace *self, struct USB2SAEAuth *auth );

#endif
