
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef USB2_SAE_CRYPTO_H
#define USB2_SAE_CRYPTO_H

typedef struct bignum_st BIGNUM;
typedef struct bignum_ctx BN_CTX;
typedef struct bn_mont_ctx_st BN_MONT_CTX;
typedef struct ec_group_st EC_GROUP;
typedef struct ec_point_st EC_POINT;

// Opaque OpenSSL vectors from the AmiSSL 5.16 and 6.1 SDKs.
#define SAE_CALL( crypto, name, ... ) \
	( ( crypto )->Version6 ? \
		( (struct SAE6IFace *) ( crypto )->SSL )->name( __VA_ARGS__ ) : \
		( (struct SAE5IFace *) ( crypto )->SSL )->name( __VA_ARGS__ ) )

struct SAE5IFace
{
	struct InterfaceData Data;
	APTR Before_BN_CTX_free[359 - 0];
	void APICALL (*BN_CTX_free)( struct SAE5IFace *, BN_CTX * );
	void APICALL (*BN_CTX_start)( struct SAE5IFace *, BN_CTX * );
	BIGNUM * APICALL (*BN_CTX_get)( struct SAE5IFace *, BN_CTX * );
	void APICALL (*BN_CTX_end)( struct SAE5IFace *, BN_CTX * );
	APTR Before_BN_bin2bn[374 - 363];
	BIGNUM * APICALL (*BN_bin2bn)( struct SAE5IFace *, const U8 *, int, BIGNUM * );
	APTR Before_BN_sub[378 - 375];
	int APICALL (*BN_sub)( struct SAE5IFace *, BIGNUM *, const BIGNUM *, const BIGNUM * );
	APTR Before_BN_nnmod[385 - 379];
	int APICALL (*BN_nnmod)( struct SAE5IFace *, BIGNUM *, const BIGNUM *, const BIGNUM *, BN_CTX * );
	int APICALL (*BN_mod_add)( struct SAE5IFace *, BIGNUM *, const BIGNUM *, const BIGNUM *, const BIGNUM *, BN_CTX * );
	APTR Before_BN_mod_mul[390 - 387];
	int APICALL (*BN_mod_mul)( struct SAE5IFace *, BIGNUM *, const BIGNUM *, const BIGNUM *, const BIGNUM *, BN_CTX * );
	APTR Before_BN_add_word[399 - 391];
	int APICALL (*BN_add_word)( struct SAE5IFace *, BIGNUM *, unsigned long );
	int APICALL (*BN_sub_word)( struct SAE5IFace *, BIGNUM *, unsigned long );
	int APICALL (*BN_set_word)( struct SAE5IFace *, BIGNUM *, unsigned long );
	APTR Before_BN_cmp[403 - 402];
	int APICALL (*BN_cmp)( struct SAE5IFace *, const BIGNUM *, const BIGNUM * );
	APTR Before_BN_rshift[417 - 404];
	int APICALL (*BN_rshift)( struct SAE5IFace *, BIGNUM *, const BIGNUM *, int );
	APTR Before_EC_GROUP_free[625 - 418];
	void APICALL (*EC_GROUP_free)( struct SAE5IFace *, EC_GROUP * );
	APTR Before_EC_POINT_new[636 - 626];
	EC_POINT * APICALL (*EC_POINT_new)( struct SAE5IFace *, const EC_GROUP * );
	APTR Before_EC_POINT_clear_free[638 - 637];
	void APICALL (*EC_POINT_clear_free)( struct SAE5IFace *, EC_POINT * );
	APTR Before_EC_POINT_add[649 - 639];
	int APICALL (*EC_POINT_add)( struct SAE5IFace *, const EC_GROUP *, EC_POINT *, const EC_POINT *, const EC_POINT *, BN_CTX * );
	APTR Before_EC_POINT_invert[651 - 650];
	int APICALL (*EC_POINT_invert)( struct SAE5IFace *, const EC_GROUP *, EC_POINT *, BN_CTX * );
	int APICALL (*EC_POINT_is_at_infinity)( struct SAE5IFace *, const EC_GROUP *, const EC_POINT * );
	int APICALL (*EC_POINT_is_on_curve)( struct SAE5IFace *, const EC_GROUP *, const EC_POINT *, BN_CTX * );
	APTR Before_EC_POINT_mul[658 - 654];
	int APICALL (*EC_POINT_mul)( struct SAE5IFace *, const EC_GROUP *, EC_POINT *, const BIGNUM *, const EC_POINT *, const BIGNUM *, BN_CTX * );
	APTR Before_BN_mod_exp_mont_consttime[2527 - 659];
	int APICALL (*BN_mod_exp_mont_consttime)( struct SAE5IFace *, BIGNUM *, const BIGNUM *, const BIGNUM *, const BIGNUM *, BN_CTX *, BN_MONT_CTX * );
	APTR Before_EC_GROUP_new_by_curve_name[2559 - 2528];
	EC_GROUP * APICALL (*EC_GROUP_new_by_curve_name)( struct SAE5IFace *, int );
	APTR Before_BN_bn2binpad[4111 - 2560];
	int APICALL (*BN_bn2binpad)( struct SAE5IFace *, const BIGNUM *, U8 *, int );
	APTR Before_BN_CTX_secure_new[4113 - 4112];
	BN_CTX * APICALL (*BN_CTX_secure_new)( struct SAE5IFace * );
	APTR Before_BN_set_flags[4130 - 4114];
	void APICALL (*BN_set_flags)( struct SAE5IFace *, BIGNUM *, int );
	APTR Before_EC_GROUP_get0_order[4171 - 4131];
	const BIGNUM * APICALL (*EC_GROUP_get0_order)( struct SAE5IFace *, const EC_GROUP * );
	APTR Before_BN_priv_rand_range[5175 - 4172];
	int APICALL (*BN_priv_rand_range)( struct SAE5IFace *, BIGNUM *, const BIGNUM * );
	APTR Before_EC_POINT_set_affine_coordinates[5359 - 5176];
	int APICALL (*EC_POINT_set_affine_coordinates)( struct SAE5IFace *, const EC_GROUP *, EC_POINT *, const BIGNUM *, const BIGNUM *, BN_CTX * );
	int APICALL (*EC_POINT_get_affine_coordinates)( struct SAE5IFace *, const EC_GROUP *, const EC_POINT *, BIGNUM *, BIGNUM *, BN_CTX * );
	APTR Before_EC_GROUP_get_curve[5362 - 5361];
	int APICALL (*EC_GROUP_get_curve)( struct SAE5IFace *, const EC_GROUP *, BIGNUM *, BIGNUM *, BIGNUM *, BN_CTX * );
};

struct SAE6IFace
{
	struct InterfaceData Data;
	APTR Before_BN_CTX_free[292 - 0];
	void APICALL (*BN_CTX_free)( struct SAE6IFace *, BN_CTX * );
	void APICALL (*BN_CTX_start)( struct SAE6IFace *, BN_CTX * );
	BIGNUM * APICALL (*BN_CTX_get)( struct SAE6IFace *, BN_CTX * );
	void APICALL (*BN_CTX_end)( struct SAE6IFace *, BN_CTX * );
	APTR Before_BN_bin2bn[304 - 296];
	BIGNUM * APICALL (*BN_bin2bn)( struct SAE6IFace *, const U8 *, int, BIGNUM * );
	APTR Before_BN_sub[308 - 305];
	int APICALL (*BN_sub)( struct SAE6IFace *, BIGNUM *, const BIGNUM *, const BIGNUM * );
	APTR Before_BN_nnmod[315 - 309];
	int APICALL (*BN_nnmod)( struct SAE6IFace *, BIGNUM *, const BIGNUM *, const BIGNUM *, BN_CTX * );
	int APICALL (*BN_mod_add)( struct SAE6IFace *, BIGNUM *, const BIGNUM *, const BIGNUM *, const BIGNUM *, BN_CTX * );
	APTR Before_BN_mod_mul[320 - 317];
	int APICALL (*BN_mod_mul)( struct SAE6IFace *, BIGNUM *, const BIGNUM *, const BIGNUM *, const BIGNUM *, BN_CTX * );
	APTR Before_BN_add_word[329 - 321];
	int APICALL (*BN_add_word)( struct SAE6IFace *, BIGNUM *, unsigned long );
	int APICALL (*BN_sub_word)( struct SAE6IFace *, BIGNUM *, unsigned long );
	int APICALL (*BN_set_word)( struct SAE6IFace *, BIGNUM *, unsigned long );
	APTR Before_BN_cmp[333 - 332];
	int APICALL (*BN_cmp)( struct SAE6IFace *, const BIGNUM *, const BIGNUM * );
	APTR Before_BN_rshift[347 - 334];
	int APICALL (*BN_rshift)( struct SAE6IFace *, BIGNUM *, const BIGNUM *, int );
	APTR Before_EC_GROUP_free[433 - 348];
	void APICALL (*EC_GROUP_free)( struct SAE6IFace *, EC_GROUP * );
	APTR Before_EC_POINT_new[440 - 434];
	EC_POINT * APICALL (*EC_POINT_new)( struct SAE6IFace *, const EC_GROUP * );
	APTR Before_EC_POINT_clear_free[442 - 441];
	void APICALL (*EC_POINT_clear_free)( struct SAE6IFace *, EC_POINT * );
	APTR Before_EC_POINT_add[447 - 443];
	int APICALL (*EC_POINT_add)( struct SAE6IFace *, const EC_GROUP *, EC_POINT *, const EC_POINT *, const EC_POINT *, BN_CTX * );
	APTR Before_EC_POINT_invert[449 - 448];
	int APICALL (*EC_POINT_invert)( struct SAE6IFace *, const EC_GROUP *, EC_POINT *, BN_CTX * );
	int APICALL (*EC_POINT_is_at_infinity)( struct SAE6IFace *, const EC_GROUP *, const EC_POINT * );
	int APICALL (*EC_POINT_is_on_curve)( struct SAE6IFace *, const EC_GROUP *, const EC_POINT *, BN_CTX * );
	APTR Before_EC_POINT_mul[453 - 452];
	int APICALL (*EC_POINT_mul)( struct SAE6IFace *, const EC_GROUP *, EC_POINT *, const BIGNUM *, const EC_POINT *, const BIGNUM *, BN_CTX * );
	APTR Before_BN_mod_exp_mont_consttime[1822 - 454];
	int APICALL (*BN_mod_exp_mont_consttime)( struct SAE6IFace *, BIGNUM *, const BIGNUM *, const BIGNUM *, const BIGNUM *, BN_CTX *, BN_MONT_CTX * );
	APTR Before_EC_GROUP_new_by_curve_name[1842 - 1823];
	EC_GROUP * APICALL (*EC_GROUP_new_by_curve_name)( struct SAE6IFace *, int );
	APTR Before_BN_bn2binpad[2775 - 1843];
	int APICALL (*BN_bn2binpad)( struct SAE6IFace *, const BIGNUM *, U8 *, int );
	APTR Before_BN_CTX_secure_new[2777 - 2776];
	BN_CTX * APICALL (*BN_CTX_secure_new)( struct SAE6IFace * );
	APTR Before_BN_set_flags[2794 - 2778];
	void APICALL (*BN_set_flags)( struct SAE6IFace *, BIGNUM *, int );
	APTR Before_EC_GROUP_get0_order[2825 - 2795];
	const BIGNUM * APICALL (*EC_GROUP_get0_order)( struct SAE6IFace *, const EC_GROUP * );
	APTR Before_BN_priv_rand_range[3531 - 2826];
	int APICALL (*BN_priv_rand_range)( struct SAE6IFace *, BIGNUM *, const BIGNUM * );
	APTR Before_EC_POINT_set_affine_coordinates[3658 - 3532];
	int APICALL (*EC_POINT_set_affine_coordinates)( struct SAE6IFace *, const EC_GROUP *, EC_POINT *, const BIGNUM *, const BIGNUM *, BN_CTX * );
	int APICALL (*EC_POINT_get_affine_coordinates)( struct SAE6IFace *, const EC_GROUP *, const EC_POINT *, BIGNUM *, BIGNUM *, BN_CTX * );
	APTR Before_EC_GROUP_get_curve[3661 - 3660];
	int APICALL (*EC_GROUP_get_curve)( struct SAE6IFace *, const EC_GROUP *, BIGNUM *, BIGNUM *, BIGNUM *, BN_CTX * );
};

#endif
