
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "AF9015.h"

// --

BOOL
AF9015_Tune( struct Receiver *receiver, U32 frequency, U32 bandwidth )
{
	U32 attempt;
	U8 status;
	BOOL tuned;

	if ( ! AF9015_Set( receiver, 0xd417, 0x08, 0x08 ) )
	{
		return ( FALSE );
	}

	if ( receiver->Tuner == AF9015_TDA18218 )
	{
		tuned = TDA18218_Tune( receiver, frequency, bandwidth );
	}
	else
	{
		tuned = MT2060_Tune( receiver, frequency );
	}

	if ( ( ! AF9015_Set( receiver, 0xd417, 0x08, 0 ) ) || ( ! tuned ) ||
		( ! AF9015_DemodTune( receiver, bandwidth ) ) )
	{
		return ( FALSE );
	}

	Printf( "Waiting for DVB-T lock at %lu Hz with %lu MHz bandwidth.\n", frequency, bandwidth / 1000000 );

	for( attempt = 0; attempt < 50; attempt++ )
	{
		if ( ( CheckSignal( SIGBREAKF_CTRL_C ) ) || ( AF9015_Detached( receiver ) ) ||
			( ! AF9015_Read( receiver, 0xd507, & status, 1 ) ) )
		{
			return ( FALSE );
		}

		if ( status & 0x40 )
		{
			Printf( "DVB-T transport stream locked.\n" );

			return ( TRUE );
		}

		Delay( 5 );
	}

	Printf( "No DVB-T signal lock. Check the antenna, frequency and bandwidth.\n" );

	return ( FALSE );
}

// --

void
AF9015_TunerSleep( struct Receiver *receiver )
{
	U8 reg;
	U8 data;

	if ( ! AF9015_Set( receiver, 0xd417, 0x08, 0x08 ) )
	{
		return;
	}

	reg = receiver->Tuner == AF9015_TDA18218 ? 0x17 : 0x0b;
	data = receiver->Tuner == AF9015_TDA18218 ? 0xf1 : 0x30;

	AF9015_I2CWrite( receiver, 0x60, reg, & data, 1 );

	AF9015_Set( receiver, 0xd417, 0x08, 0 );
}

// --
