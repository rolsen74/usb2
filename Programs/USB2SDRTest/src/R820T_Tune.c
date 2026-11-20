
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <proto/dos.h>
#include <string.h>
#include "SDRTest.h"

// --

/*
** Register settings adapted from Osmocom rtl-sdr's tuner_r82xx.c.
** Copyright (C) 2013 Mauro Carvalho Chehab <mchehab@redhat.com>
** Copyright (C) 2013 Steve Markgraf <steve@steve-m.de>
*/

#define RTL_I2C_READ	0x0600
#define RTL_I2C_WRITE	0x0610
#define R820T_FIRST_REG	0x05
#define R820T_LAST_REG	0x1f
#define R820T_VCO_MIN	1770000000ULL
#define R820T_WRITE_MAX	7U

struct R820T
{
	struct USB2_EPResource *Control;
	U8 Registers[R820T_LAST_REG + 1];
};

struct RegisterSetting
{
	U8 Register;
	U8 Mask;
	U8 Value;
};

static const U8 InitialRegisters[] =
{
	0x83, 0x32, 0x75, 0xc0, 0x40, 0xd6, 0x6c, 0xf5, 0x63,
	0x75, 0x68, 0x6c, 0x83, 0x80, 0x00, 0x0f, 0x00, 0xc0,
	0x30, 0x48, 0xcc, 0x60, 0x00, 0x54, 0xae, 0x4a, 0xc0,
};

// --

static S32
WriteRegisters( struct R820T *tuner, U8 reg, const U8 *values, U16 count )
{
	U8 data[R820T_WRITE_MAX + 1];
	U16 bytes;
	S32 error;

	while ( count )
	{
		bytes = MIN( count, R820T_WRITE_MAX );
		data[0] = reg;

		memcpy( data + 1, values, bytes );

		error = SDR_Control( tuner->Control, SDR_R820T_ADDRESS, RTL_I2C_WRITE, data, bytes + 1 );

		if ( error )
		{
			Printf( "R820T register $%02lx write failed (USB error %ld).\n", (U32) reg, error );

			return ( error );
		}

		memcpy( tuner->Registers + reg, values, bytes );

		reg += bytes;
		values += bytes;
		count -= bytes;
	}

	return ( 0 );
}

// --

static S32
WriteMasked( struct R820T *tuner, U8 reg, U8 mask, U8 value )
{
	value = ( tuner->Registers[reg] & ~mask ) | ( value & mask );

	if ( value == tuner->Registers[reg] )
	{
		return ( 0 );
	}

	return ( WriteRegisters( tuner, reg, &value, 1 ) );
}

// --

static S32
ReadStatus( struct R820T *tuner, U8 *status, U16 count )
{
	U16 index;
	U8 reg;
	U8 byte;
	S32 error;

	reg = 0;

	error = SDR_Control( tuner->Control, SDR_R820T_ADDRESS, RTL_I2C_WRITE, &reg, 1 );

	if ( ! error )
	{
		error = SDR_Control( tuner->Control, SDR_R820T_ADDRESS, RTL_I2C_READ, status, count );
	}

	if ( error )
	{
		Printf( "R820T status read failed (USB error %ld).\n", error );

		return ( error );
	}

	// The tuner reports its status bytes least-significant bit first.
	for ( index = 0; index < count; index++ )
	{
		byte = status[index];
		byte = ( ( byte & 0x55 ) << 1 ) | ( ( byte >> 1 ) & 0x55 );
		byte = ( ( byte & 0x33 ) << 2 ) | ( ( byte >> 2 ) & 0x33 );
		status[index] = ( byte << 4 ) | ( byte >> 4 );
	}

	return ( 0 );
}

// --

static S32
SetPLL( struct R820T *tuner, U32 frequency )
{
	U64 vco;
	U32 divider;
	U32 dividerCode;
	U32 fine;
	U32 scaled;
	U32 integer;
	U16 fraction;
	U8 status[5];
	U8 values[7];

	if ( WriteMasked( tuner, 0x1a, 0x0c, 0x00 ) )
	{
		return ( FALSE );
	}

	vco = (U64) frequency * 2;
	dividerCode = 0;

	while ( ( vco < R820T_VCO_MIN ) && ( dividerCode < 5 ) )
	{
		vco *= 2;
		dividerCode++;
	}

	if ( ( vco < R820T_VCO_MIN ) || ( vco >= 2 * R820T_VCO_MIN ) )
	{
		Printf( "R820T cannot synthesize %lu Hz.\n", frequency );

		return ( FALSE );
	}

	if ( ReadStatus( tuner, status, sizeof( status ) ) )
	{
		return ( FALSE );
	}

	fine = ( status[4] >> 4 ) & 3;

	if ( ( fine > 2 ) && ( dividerCode ) )
	{
		dividerCode--;
	}
	else if ( ( fine < 2 ) && ( dividerCode < 7 ) )
	{
		dividerCode++;
	}

	// The VCO feedback divider is an integer and a 16-bit fraction.
	scaled = ( ( vco << 16 ) + SDR_CLOCK_HZ ) / ( 2 * SDR_CLOCK_HZ );
	integer = scaled >> 16;
	fraction = scaled;

	if ( ( integer < 13 ) || ( integer > 63 ) )
	{
		Printf( "R820T PLL divider is out of range for %lu Hz.\n", frequency );

		return ( FALSE );
	}

	divider = integer - 13;

	memcpy( values, tuner->Registers + 0x10, sizeof( values ) );

	values[0] = ( values[0] & 0x0f ) | ( dividerCode << 5 );
	values[2] = ( values[2] & 0x17 ) | 0x80 | ( fraction ? 0 : 0x08 );
	values[4] = ( divider / 4 ) | ( ( divider % 4 ) << 6 );
	values[5] = fraction;
	values[6] = fraction >> 8;

	if ( WriteRegisters( tuner, 0x10, values, sizeof( values ) ) )
	{
		return ( FALSE );
	}

	Delay( 1 );

	if ( ReadStatus( tuner, status, 3 ) )
	{
		return ( FALSE );
	}

	if ( ! ( status[2] & 0x40 ) )
	{
		if ( WriteMasked( tuner, 0x12, 0xe0, 0x60 ) )
		{
			return ( FALSE );
		}

		Delay( 1 );

		if ( ReadStatus( tuner, status, 3 ) )
		{
			return ( FALSE );
		}

		if ( ! ( status[2] & 0x40 ) )
		{
			Printf( "R820T PLL did not lock at %lu Hz.\n", frequency );

			return ( FALSE );
		}
	}

	return ( ! WriteMasked( tuner, 0x1a, 0x08, 0x08 ) );
}

// --

static S32
CalibrateFilter( struct R820T *tuner )
{
	U8 status[5];
	U8 code;
	U32 attempt;

	code = 0;

	for ( attempt = 0; attempt < 2; attempt++ )
	{
		if ( ( WriteMasked( tuner, 0x0b, 0x60, 0x60 ) ) ||
			( WriteMasked( tuner, 0x0f, 0x04, 0x04 ) ) ||
			( WriteMasked( tuner, 0x10, 0x03, 0x00 ) ) ||
			( ! SetPLL( tuner, 56000000 ) ) ||
			( WriteMasked( tuner, 0x0b, 0x10, 0x10 ) ) )
		{
			return ( FALSE );
		}

		Delay( 1 );

		if ( ( WriteMasked( tuner, 0x0b, 0x10, 0x00 ) ) ||
			( WriteMasked( tuner, 0x0f, 0x04, 0x00 ) ) ||
			( ReadStatus( tuner, status, sizeof( status ) ) ) )
		{
			return ( FALSE );
		}

		code = status[4] & 0x0f;

		if ( ( code ) && ( code != 0x0f ) )
		{
			break;
		}
	}

	if ( code == 0x0f )
	{
		code = 0;
	}

	return ( ! WriteMasked( tuner, 0x0a, 0x1f, 0x10 | code ) );
}

// --

S32
SDR_R820T_Tune( struct USB2_EPResource *control, U32 frequency )
{
	static const struct RegisterSetting settings[] =
	{
		// 3.57 MHz IF and 6 MHz analog filter.
		{ 0x0b, 0xef, 0x6b }, { 0x07, 0x80, 0x00 },
		{ 0x06, 0x30, 0x10 }, { 0x1e, 0x60, 0x60 },
		{ 0x05, 0x80, 0x00 }, { 0x1f, 0x80, 0x00 },
		{ 0x0f, 0x80, 0x00 }, { 0x19, 0x60, 0x60 },
		// VHF input and automatic LNA and mixer gain.
		{ 0x1d, 0xff, 0xdd }, { 0x1c, 0xfc, 0x24 },
		{ 0x0d, 0xff, 0x53 }, { 0x0e, 0xff, 0x75 },
		{ 0x05, 0x70, 0x00 }, { 0x06, 0x48, 0x00 },
		{ 0x11, 0x38, 0x38 }, { 0x17, 0x38, 0x30 },
		{ 0x0a, 0x60, 0x40 }, { 0x1e, 0x1f, 0x0e },
		{ 0x1a, 0xf3, 0x22 }, { 0x07, 0x10, 0x10 },
		{ 0x0c, 0x9f, 0x0b }, { 0x10, 0x0b, 0x00 },
		{ 0x08, 0x3f, 0x00 }, { 0x09, 0x3f, 0x00 },
	};

	struct R820T tuner;

	U32 index;
	U32 localOscillator;
	U8 filter;

	tuner.Control = control;
	localOscillator = frequency + SDR_IF_HZ;
	filter = ( localOscillator < 110000000U ) ? 0x34 : 0x24;

	if ( ( WriteRegisters( &tuner, R820T_FIRST_REG, InitialRegisters, sizeof( InitialRegisters ) ) ) ||
		( WriteMasked( &tuner, 0x0c, 0x0f, 0x00 ) ) ||
		( WriteMasked( &tuner, 0x13, 0x3f, 49 ) ) ||
		( WriteMasked( &tuner, 0x1d, 0x38, 0x00 ) ) ||
		( ! CalibrateFilter( &tuner ) ) )
	{
		return ( FALSE );
	}

	for ( index = 0; index < sizeof( settings ) / sizeof( settings[0] ); index++ )
	{
		if ( WriteMasked( &tuner, settings[index].Register, settings[index].Mask, settings[index].Value ) )
		{
			return ( FALSE );
		}
	}

	if ( WriteMasked( &tuner, 0x1b, 0xff, filter ) )
	{
		return ( FALSE );
	}

	return ( SetPLL( &tuner, localOscillator ) );
}

// --

S32
SDR_R820T_Standby( struct USB2_EPResource *control )
{
	static const U8 registers[][2] =
	{
		{ 0x06, 0xb1 }, { 0x05, 0xa0 }, { 0x07, 0x3a }, { 0x08, 0x40 },
		{ 0x09, 0xc0 }, { 0x0a, 0x36 }, { 0x0c, 0x35 }, { 0x0f, 0x68 },
		{ 0x11, 0x03 }, { 0x17, 0xf4 }, { 0x19, 0x0c },
	};

	struct R820T tuner;

	U32 index;

	tuner.Control = control;

	for ( index = 0; index < sizeof( registers ) / sizeof( registers[0] ); index++ )
	{
		if ( WriteRegisters( &tuner, registers[index][0], &registers[index][1], 1 ) )
		{
			return ( FALSE );
		}
	}

	return ( TRUE );
}

// --
