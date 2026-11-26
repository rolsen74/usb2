
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
** Tuner sequences from the Linux driver, Copyright (C) 2010 Antti Palosaari
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "AF9015.h"

// --

#define TDA_ADDRESS 0x60

static const U8 Defaults[] =
{
	0xc0, 0x88, 0x00, 0x8e, 0x03, 0x00, 0x00, 0xd0, 0x00, 0x40,
	0x00, 0x00, 0x07, 0xff, 0x84, 0x09, 0x00, 0x13, 0x00, 0x00,
	0x01, 0x84, 0x09, 0xf0, 0x19, 0x0a, 0x8e, 0x69, 0x98, 0x01,
	0x00, 0x58, 0x10, 0x40, 0x8c, 0x00, 0x0c, 0x48, 0x85, 0xc9,
	0xa7, 0x00, 0x00, 0x00, 0x30, 0x81, 0x80, 0x00, 0x39, 0x00,
	0x8a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf6, 0xf6
};

// --

BOOL
TDA18218_Tune( struct Receiver *receiver, U32 frequency, U32 bandwidth )
{
	static const U8 agc[][2] =
	{
		{ 0x20, 0x60 }, { 0x23, 0x02 }, { 0x20, 0xa0 }, { 0x23, 0x09 },
		{ 0x20, 0xe0 }, { 0x23, 0x0c }, { 0x20, 0x40 }, { 0x23, 0x01 },
		{ 0x20, 0x80 }, { 0x23, 0x08 }, { 0x20, 0xc0 }, { 0x23, 0x0b },
		{ 0x24, 0x1c }, { 0x24, 0x0c }
	};

	U32 local;
	U32 index;
	U8 buffer[3];
	U8 id;
	U8 filter;
	U8 cutoff;

	if ( ( ! AF9015_I2CRead( receiver, TDA_ADDRESS, 0, & id, 1 ) ) || ( id != Defaults[0] ) )
	{
		Printf( "TDA18218 chip identification failed.\n" );

		return ( FALSE );
	}

	receiver->TunerDetected = TRUE;

	for( index = 0; index < sizeof( Defaults ); index += 20 )
	{
		if ( ! AF9015_I2CWrite( receiver, TDA_ADDRESS, index, & Defaults[index],
			MIN( 20U, sizeof( Defaults ) - index ) ) )
		{
			return ( FALSE );
		}
	}

	cutoff = bandwidth / 1000000 - 6;
	receiver->IFrequency = 3000000 + cutoff * 500000;
	local = frequency + receiver->IFrequency;

	if ( local < 188000000 )
	{
		filter = 3;
	}
	else if ( local < 253000000 )
	{
		filter = 4;
	}
	else if ( local < 343000000 )
	{
		filter = 5;
	}
	else
	{
		filter = 6;
	}

	buffer[0] = ( Defaults[0x1a] & ~7 ) | filter;
	buffer[1] = ( Defaults[0x1b] & ~3 ) | cutoff;
	buffer[2] = Defaults[0x1c];

	if ( ! AF9015_I2CWrite( receiver, TDA_ADDRESS, 0x1a, buffer, sizeof( buffer ) ) )
	{
		return ( FALSE );
	}

	local /= 1000;
	buffer[0] = local >> 12;
	buffer[1] = local >> 4;
	buffer[2] = ( local << 4 ) | ( Defaults[0x0c] & 0x0f );

	if ( ! AF9015_I2CWrite( receiver, TDA_ADDRESS, 0x0a, buffer, sizeof( buffer ) ) )
	{
		return ( FALSE );
	}

	buffer[0] = Defaults[0x0f] | 0x40;

	if ( ! AF9015_I2CWrite( receiver, TDA_ADDRESS, 0x0f, buffer, 1 ) )
	{
		return ( FALSE );
	}

	buffer[0] = Defaults[0x0f] & ~0x40;

	if ( ! AF9015_I2CWrite( receiver, TDA_ADDRESS, 0x0f, buffer, 1 ) )
	{
		return ( FALSE );
	}

	for( index = 0; index < sizeof( agc ) / sizeof( agc[0] ); index++ )
	{
		if ( ! AF9015_I2CWrite( receiver, TDA_ADDRESS, agc[index][0], & agc[index][1], 1 ) )
		{
			return ( FALSE );
		}
	}

	return ( TRUE );
}

// --
