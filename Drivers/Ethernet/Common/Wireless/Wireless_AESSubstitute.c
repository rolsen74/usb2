
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <string.h>
#include "WirelessCrypto.h"

// --

static U32
Multiply( U32 a, U32 b )
{
	U32 product;
	U32 high;
	U32 i;

	product = 0;

	// Apply GF(2^8) multiplication to four independent byte lanes.
	for( i = 0; i < 8; i++ )
	{
		product ^= a & ( ( b & 0x01010101 ) * 0xff );
		high = ( a >> 7 ) & 0x01010101;
		a = ( ( a << 1 ) & 0xfefefefe ) ^ ( high * 0x1b );
		b >>= 1;
	}

	return( product );
}

// --

U32
Wireless_AESSubstitute( U32 value )
{
	U32 square;
	U32 fourth;
	U32 eighth;
	U32 inverse;

	// Multiplicative inverse (x^254), followed by the AES affine transform.
	square = Multiply( value, value );
	fourth = Multiply( square, square );
	eighth = Multiply( fourth, fourth );
	inverse = Multiply( square, fourth );

	square = Multiply( eighth, eighth );
	fourth = Multiply( square, square );
	inverse = Multiply( inverse, eighth );
	inverse = Multiply( inverse, square );
	inverse = Multiply( inverse, fourth );

	square = Multiply( fourth, fourth );
	fourth = Multiply( square, square );
	inverse = Multiply( inverse, square );
	inverse = Multiply( inverse, fourth );

	return( inverse ^ 0x63636363 ^
		( ( ( inverse << 1 ) & 0xfefefefe ) | ( ( inverse >> 7 ) & 0x01010101 ) ) ^
		( ( ( inverse << 2 ) & 0xfcfcfcfc ) | ( ( inverse >> 6 ) & 0x03030303 ) ) ^
		( ( ( inverse << 3 ) & 0xf8f8f8f8 ) | ( ( inverse >> 5 ) & 0x07070707 ) ) ^
		( ( ( inverse << 4 ) & 0xf0f0f0f0 ) | ( ( inverse >> 4 ) & 0x0f0f0f0f ) ) );
}

// --
