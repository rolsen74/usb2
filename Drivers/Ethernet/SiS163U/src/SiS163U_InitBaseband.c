
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define SIS163U_BBP_CTRL	0x001c

// --

S32
SiS163U_WriteBaseband( struct USB2_EPResource *control, U8 address, U8 value, U32 ofdm )
{
	S32 error;

	error = SiS163U_WriteRegister( control, SIS163U_BBP_CTRL,
		( (U32) value << 16 ) | ( (U32) address << 8 ) | ( ofdm ? 0x30 : 0x20 ) );

	if ( error )
	{
		return ( error );
	}

	return ( SiS163U_WaitRegister( control, SIS163U_BBP_CTRL, 0x40 ) );
}

// --

S32
SiS163U_InitBaseband( struct USB2_EPResource *control )
{
	// RF type 8 baseband profile.
	static const U8 ofdm[][2] =
	{
		{ 0x40, 0x34 }, { 0x42, 0x88 }, { 0x44, 0x65 }, { 0x4c, 0x01 },
		{ 0x4e, 0x00 }, { 0x56, 0x87 }, { 0x5c, 0x06 }, { 0x70, 0x68 },
		{ 0x72, 0x83 }, { 0x74, 0x96 }, { 0x76, 0x80 }, { 0x78, 0x38 },
		{ 0x7a, 0x3f }, { 0x7c, 0x21 }, { 0x7e, 0x05 }, { 0x80, 0x60 },
		{ 0x82, 0x06 }, { 0x84, 0x08 }, { 0x86, 0x27 }, { 0x88, 0x0a },
		{ 0x8a, 0x09 }, { 0x8c, 0x08 }, { 0x8e, 0x23 }, { 0x90, 0x2a },
		{ 0x92, 0x31 }, { 0x94, 0xca }, { 0x96, 0x39 }, { 0x98, 0x37 },
		{ 0x9a, 0x30 }, { 0x9c, 0x12 }, { 0x9e, 0x64 }, { 0xc0, 0x03 },
		{ 0xc2, 0x00 }, { 0xd0, 0x80 },
	};
	static const U8 cck[][2] =
	{
		{ 0x2e, 0x03 }, { 0x36, 0x0d }, { 0x38, 0x0a }, { 0x4a, 0x0f },
	};

	U32 index;
	U32 readback;
	S32 error;

	for( index = 0; index < sizeof( ofdm ) / sizeof( ofdm[0] ); index++ )
	{
		error = SiS163U_WriteBaseband( control, ofdm[index][0], ofdm[index][1], TRUE );

		if ( error )
		{
			return ( error );
		}
	}

	for( index = 0; index < sizeof( cck ) / sizeof( cck[0] ); index++ )
	{
		error = SiS163U_WriteBaseband( control, cck[index][0], cck[index][1], FALSE );

		if ( error )
		{
			return ( error );
		}
	}

	// Read an OFDM setting back through the baseband command port.
	error = SiS163U_WriteRegister( control, SIS163U_BBP_CTRL, 0x0074b0 );

	if ( ! error )
	{
		error = SiS163U_WaitRegister( control, SIS163U_BBP_CTRL, 0x40 );
	}

	if ( ! error )
	{
		error = SiS163U_ReadRegister( control, SIS163U_BBP_CTRL, & readback );
	}

	if ( error )
	{
		return ( error );
	}

	USBINFO( "SiS163U: OFDM register $74 read back $%02lx (wrote $96)", ( readback >> 16 ) & 0xff );

	return ( 0 );
}

// --
