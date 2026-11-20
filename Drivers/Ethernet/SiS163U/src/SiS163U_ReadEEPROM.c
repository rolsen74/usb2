
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <interfaces/timer.h>
#include "All.h"

// --

#define SIS163U_EEPROM_CTRL	0x0008
#define SIS163U_EEPROM_BUSY	0x80
#define SIS163U_EEPROM_POLLS	64
#define SIS163U_EEPROM_PINS	0x28
#define SIS163U_EEPROM_CLK	0x04
#define SIS163U_EEPROM_DI	0x01
#define SIS163U_EEPROM_DO	0x02

// --

static S32
WritePins( struct USB2_EPResource *control, struct TimerIFace *timer, U32 pins )
{
	S32 error;

	error = SiS163U_WriteRegister( control, SIS163U_EEPROM_CTRL, pins );

	if ( ! error )
	{
		timer->MicroDelay( 200 );
	}

	return ( error );
}

// --

static S32
ReadEEPROMBits( struct USB2_EPResource *control, struct TimerIFace *timer, U8 word, U32 *data )
{
	U32 command;
	U32 mask;
	U32 pins;
	U32 bit;
	U32 value;
	S32 error;

	command = 0x180 | word;
	value = 0;

	for( mask = 0x100; mask; mask >>= 1 )
	{
		pins = SIS163U_EEPROM_PINS | ( ( command & mask ) ? SIS163U_EEPROM_DI : 0 );

		error = WritePins( control, timer, pins );

		if ( error )
		{
			return ( error );
		}

		error = WritePins( control, timer, pins | SIS163U_EEPROM_CLK );

		if ( error )
		{
			return ( error );
		}
	}

	for( mask = 0x8000; mask; mask >>= 1 )
	{
		error = WritePins( control, timer, SIS163U_EEPROM_PINS );

		if ( error )
		{
			return ( error );
		}

		error = WritePins( control, timer, SIS163U_EEPROM_PINS | SIS163U_EEPROM_CLK );

		if ( error )
		{
			return ( error );
		}

		error = SiS163U_ReadRegister( control, SIS163U_EEPROM_CTRL, & bit );

		if ( error )
		{
			return ( error );
		}

		if ( bit & SIS163U_EEPROM_DO )
		{
			value |= mask;
		}
	}

	*data = value;

	return ( 0 );
}

// --

static S32
ReadEEPROMHardware( struct USB2_EPResource *control, U8 word, U32 *data )
{
	U32 poll;
	U32 value;
	S32 error;

	error = SiS163U_WriteRegister( control, SIS163U_EEPROM_CTRL, ( (U32) word << 8 ) | SIS163U_EEPROM_BUSY );

	if ( error )
	{
		return ( error );
	}

	for( poll = 0; poll < SIS163U_EEPROM_POLLS; poll++ )
	{
		error = SiS163U_ReadRegister( control, SIS163U_EEPROM_CTRL, & value );

		if ( error )
		{
			return ( error );
		}

		if ( ! ( value & SIS163U_EEPROM_BUSY ) )
		{
			error = SiS163U_ReadRegister( control, SIS163U_EEPROM_CTRL, & value );

			if ( ! error )
			{
				*data = value >> 16;
			}

			return ( error );
		}
	}

	return ( USB2Err_Host_Timeout );
}

// --

S32
SiS163U_ReadEEPROM( struct USB2_EPResource *control, struct TimerIFace *timer, U8 word, U16 *data )
{
	U32 value;
	S32 error;
	S32 reset;

	value = 0;

	if ( timer )
	{
		error = ReadEEPROMBits( control, timer, word, & value );

		reset = WritePins( control, timer, 0 );
	}
	else
	{
		error = ReadEEPROMHardware( control, word, & value );

		reset = SiS163U_WriteRegister( control, SIS163U_EEPROM_CTRL, 0 );
	}

	if ( ( ! error ) && ( ! reset ) )
	{
		*data = value;
	}

	return ( error ? error : reset );
}

// --
