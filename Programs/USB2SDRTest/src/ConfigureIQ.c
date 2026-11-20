
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "SDRTest.h"

// --

/*
** RTL2832U settings adapted from Osmocom rtl-sdr's librtlsdr.c.
** Copyright (C) 2012-2014 Steve Markgraf <steve@steve-m.de>
** Copyright (C) 2012 Dimitri Stolnikov <horiz0n@gmx.net>
*/

#define RTL_USB_BLOCK		0x0100
#define RTL_USB_EPA_CTL		0x2148
#define RTL_USB_EPA_MAXPKT	0x2158

struct DemodSetting
{
	U8 Page;
	U8 Register;
	U8 Value;
};

// --

S32
SDR_HoldFIFO( struct USB2_EPResource *control, S32 reset )
{
	return ( SDR_WriteRegister( control, RTL_USB_EPA_CTL, RTL_USB_BLOCK, reset ? 0x1002 : 0, 2 ) );
}

// --

S32
SDR_ConfigureIQ( struct USB2_EPResource *control )
{
	static const U8 coefficients[] =
	{
		// Default FM/DAB filter, packed into eight 8-bit and eight 12-bit coefficients.
		0xca, 0xdc, 0xd7, 0xd8, 0xe0, 0xf2, 0x0e, 0x35,
		0x06, 0x50, 0x9c, 0x0d, 0x71, 0x11,
		0x14, 0x71, 0x74, 0x19, 0x41, 0xa5,
	};

	static const struct DemodSetting settings[] =
	{
		// SDR output, FSM state, AGC loops and PID filter.
		{ 0, 0x19, 0x05 }, { 1, 0x93, 0xf0 }, { 1, 0x94, 0x0f },
		{ 1, 0x11, 0x00 }, { 1, 0x04, 0x00 }, { 0, 0x61, 0x60 },
		// R820T low-IF with spectrum inversion and the I ADC enabled.
		{ 0, 0x06, 0x80 }, { 1, 0xb1, 0x1a }, { 0, 0x08, 0x4d },
		{ 1, 0x15, 0x01 }, { 0, 0x0d, 0x83 },
		{ 1, 0x3f, 0x00 }, { 1, 0x3e, 0x00 },
	};

	U32 index;
	U32 shift;
	U32 ratio;

	if ( ( SDR_WriteRegister( control, RTL_USB_EPA_MAXPKT, RTL_USB_BLOCK, 0x0002, 2 ) ) ||
		( SDR_HoldFIFO( control, TRUE ) ) ||
		( SDR_WriteDemod( control, 1, 0x01, 0x14, 1 ) ) ||
		( SDR_WriteDemod( control, 1, 0x01, 0x10, 1 ) ) )
	{
		return ( FALSE );
	}

	for ( index = 0; index < 6; index++ )
	{
		if ( SDR_WriteDemod( control, 1, 0x16 + index, 0, 1 ) )
		{
			return ( FALSE );
		}
	}

	for ( index = 0; index < sizeof( coefficients ); index++ )
	{
		if ( SDR_WriteDemod( control, 1, 0x1c + index, coefficients[index], 1 ) )
		{
			return ( FALSE );
		}
	}

	for ( index = 0; index < sizeof( settings ) / sizeof( settings[0] ); index++ )
	{
		if ( SDR_WriteDemod( control, settings[index].Page, settings[index].Register, settings[index].Value, 1 ) )
		{
			return ( FALSE );
		}
	}

	shift = -(S32) ( ( (U64) SDR_IF_HZ << 22 ) / SDR_CLOCK_HZ );
	ratio = ( ( (U64) SDR_CLOCK_HZ << 22 ) / SDR_SAMPLE_RATE ) & 0x0ffffffc;

	if ( ( SDR_WriteDemod( control, 1, 0x19, ( shift >> 16 ) & 0x3f, 1 ) ) ||
		( SDR_WriteDemod( control, 1, 0x1a, shift >> 8, 1 ) ) ||
		( SDR_WriteDemod( control, 1, 0x1b, shift, 1 ) ) ||
		( SDR_WriteDemod( control, 1, 0x9f, ratio >> 16, 2 ) ) ||
		( SDR_WriteDemod( control, 1, 0xa1, ratio, 2 ) ) ||
		( SDR_WriteDemod( control, 1, 0x01, 0x14, 1 ) ) ||
		( SDR_WriteDemod( control, 1, 0x01, 0x10, 1 ) ) )
	{
		return ( FALSE );
	}

	return ( TRUE );
}

// --
