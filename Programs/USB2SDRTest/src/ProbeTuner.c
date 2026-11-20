
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <proto/dos.h>
#include "SDRTest.h"

// --

#define RTL_I2C_REG_READ 0x0300

struct TunerProbe
{
	const char *Name;
	U16 Register;
	U8 Mask;
	U8 ID;
};

static const struct TunerProbe Tuners[] =
{
	{ "Rafael Micro R820T family",	SDR_R820T_ADDRESS, 0xff, 0x69 },
	{ "Rafael Micro R828D",		0x0074, 0xff, 0x69 },
	{ "Elonics E4000",		0x02c8, 0xff, 0x40 },
	{ "Fitipower FC0013",		0x00c6, 0xff, 0xa3 },
	{ "Fitipower FC0012",		0x00c6, 0xff, 0xa1 },
	{ "FCI FC2580",			0x01ac, 0x7f, 0x56 },
};

// --

S32
SDR_ProbeTuner( struct USB2_EPResource *control, U8 *tunerAddress )
{
	const struct TunerProbe *tuner;

	U32 index;
	U16 previous;
	U8 id;
	S32 error;
	S32 result;

	*tunerAddress = 0;

	if ( SDR_PowerOn( control ) )
	{
		return ( RETURN_FAIL );
	}

	result = RETURN_FAIL;

	if ( SDR_SetI2CGate( control, TRUE ) )
	{
		goto close_gate;
	}

	result = RETURN_WARN;
	previous = 0xffff;
	error = 0;
	id = 0;

	for ( index = 0; index < sizeof( Tuners ) / sizeof( Tuners[0] ); index++ )
	{
		tuner = &Tuners[index];

		if ( tuner->Register != previous )
		{
			previous = tuner->Register;

			error = SDR_Control( control, tuner->Register, RTL_I2C_REG_READ, &id, 1 );

			if ( error == USB2Err_Device_Detached )
			{
				Printf( "The receiver disconnected during the probe.\n" );

				return ( RETURN_FAIL );
			}

			if ( error )
			{
				Printf( "I2C $%02lx register $%02lx read failed (USB error %ld).\n",
					(U32) ( previous & 0xff ), (U32) ( previous >> 8 ), error );
			}
			else
			{
				Printf( "I2C $%02lx register $%02lx returned $%02lx.\n",
					(U32) ( previous & 0xff ), (U32) ( previous >> 8 ), (U32) id );
			}
		}

		if ( ( ! error ) && ( ( id & tuner->Mask ) == tuner->ID ) )
		{
			Printf( "Tuner: %s\n", tuner->Name );

			*tunerAddress = tuner->Register & 0xff;
			result = RETURN_OK;
			break;
		}
	}

	if ( result == RETURN_WARN )
	{
		Printf( "No known tuner ID matched.\n" );
	}

close_gate:
	if ( ( SDR_SetI2CGate( control, FALSE ) ) && ( result == RETURN_OK ) )
	{
		result = RETURN_WARN;
	}

	if ( ( SDR_PowerOff( control ) ) && ( result == RETURN_OK ) )
	{
		result = RETURN_WARN;
	}

	return ( result );
}

// --
