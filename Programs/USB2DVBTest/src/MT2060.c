
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
** Tuner sequences from the Linux MT2060 driver
** Copyright (C) 2006 Olivier Danet
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "AF9015.h"

// --

#define MT_ADDRESS 0x60

enum
{
	PART_REV = 0,
	LO1C1 = 1,
	LO2C1 = 3,
	LO_STATUS = 6,
	MISC_STAT = 8,
	MISC_CTRL = 9,
	VGAG = 0x0b,
	LO1B1 = 0x0c,
	IF2_KHZ = 36150
};

// --

static BOOL
SetRegister( struct Receiver *receiver, U8 reg, U8 value )
{
	return ( AF9015_I2CWrite( receiver, MT_ADDRESS, reg, & value, 1 ) );
}

// --

static BOOL
Calibrate( struct Receiver *receiver )
{
	static const U8 lo[] = { 0x3f, 0x74, 0x00, 0x08, 0x93 };
	static const U8 misc[] = { 0x20, 0x1e, 0x30, 0xff, 0x80, 0xff, 0x00, 0x2c, 0x42 };

	U32 attempt;
	U8 status;

	if ( ( ! AF9015_I2CWrite( receiver, MT_ADDRESS, LO1C1, lo, sizeof( lo ) ) ) ||
		( ! AF9015_I2CWrite( receiver, MT_ADDRESS, MISC_CTRL, misc, sizeof( misc ) ) ) ||
		( ! SetRegister( receiver, VGAG, 0x30 ) ) )
	{
		return ( FALSE );
	}

	for( attempt = 0; attempt < 9; attempt++ )
	{
		if ( ! SetRegister( receiver, LO2C1, 0x40 ) )
		{
			return ( FALSE );
		}

		Delay( 1 );

		if ( ! attempt )
		{
			if ( ! SetRegister( receiver, LO2C1, 0xc0 ) )
			{
				return ( FALSE );
			}

			Delay( 1 );
		}

		if ( ! SetRegister( receiver, LO2C1, 0 ) )
		{
			return ( FALSE );
		}

		Delay( 1 );
	}

	for( attempt = 0; attempt < 10; attempt++ )
	{
		if ( ! AF9015_I2CRead( receiver, MT_ADDRESS, MISC_STAT, & status, 1 ) )
		{
			return ( FALSE );
		}

		if ( status & 0x40 )
		{
			return ( SetRegister( receiver, VGAG, 0x33 ) );
		}

		Delay( 1 );
	}

	Printf( "MT2060 calibration timed out.\n" );

	return ( FALSE );
}

// --

BOOL
MT2060_Tune( struct Receiver *receiver, U32 frequency )
{
	static const U32 bands[] = { 95000, 180000, 260000, 335000, 425000, 480000, 570000, 645000, 730000, 810000 };

	U32 band;
	U32 lo1;
	U32 lo2;
	U32 num1;
	U32 num2;
	U32 attempt;
	U8 pll[5];
	U8 status;

	if ( ( ! AF9015_I2CRead( receiver, MT_ADDRESS, PART_REV, & status, 1 ) ) || ( status != 0x63 ) )
	{
		Printf( "MT2060 chip identification failed.\n" );

		return ( FALSE );
	}

	receiver->TunerDetected = TRUE;

	if ( ( receiver->MT2060_IF1 < 1000 ) || ( receiver->MT2060_IF1 > 1400 ) )
	{
		Printf( "MT2060 EEPROM IF1 is out of range (%lu MHz).\n", (U32) receiver->MT2060_IF1 );

		return ( FALSE );
	}

	if ( ( ! Calibrate( receiver ) ) || ( ! SetRegister( receiver, LO1B1, 0xff ) ) )
	{
		return ( FALSE );
	}

	frequency /= 1000;
	lo1 = ( ( frequency + receiver->MT2060_IF1 * 1000U ) / 250 ) * 250;
	lo2 = ( ( lo1 - frequency - IF2_KHZ + 25 ) / 50 ) * 50;
	num1 = lo1 / 250;
	num2 = lo2 * 64 / 125;
	band = 0;

	while( ( band < sizeof( bands ) / sizeof( bands[0] ) ) && ( frequency > bands[band] ) )
	{
		band++;
	}

	pll[0] = ( ( 11 - band ) << 4 ) | ( ( num1 >> 2 ) & 0x0f );
	pll[1] = num1 >> 6;
	pll[2] = ( num2 & 0x0f ) | ( ( num1 & 3 ) << 4 );
	pll[3] = num2 >> 4;
	pll[4] = ( ( num2 >> 12 ) & 1 ) | ( ( num2 >> 13 ) << 1 );

	if ( ! AF9015_I2CWrite( receiver, MT_ADDRESS, LO1C1, pll, sizeof( pll ) ) )
	{
		return ( FALSE );
	}

	for( attempt = 0; attempt < 10; attempt++ )
	{
		if ( ! AF9015_I2CRead( receiver, MT_ADDRESS, LO_STATUS, & status, 1 ) )
		{
			return ( FALSE );
		}

		if ( ( status & 0x88 ) == 0x88 )
		{
			receiver->IFrequency = IF2_KHZ * 1000U;

			return ( TRUE );
		}

		Delay( 1 );
	}

	Printf( "MT2060 PLL did not lock.\n" );

	return ( FALSE );
}

// --
