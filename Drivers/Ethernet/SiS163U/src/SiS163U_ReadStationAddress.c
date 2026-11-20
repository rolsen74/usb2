
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

#define SIS163U_EEPROM_SIGNATURE 0x0160
#define SIS163U_EEPROM_MAC_WORD 8
#define SIS163U_EEPROM_BOARD_WORD 0x18
#define SIS163U_EEPROM_OPTIONS_WORD 0x21
#define SIS163U_EEPROM_RF_WORD 0x28

// --

S32
SiS163U_ReadStationAddress( struct EthernetUnit *unit )
{
	struct USB2_EPResource *control;
	struct TimerIFace *timer;
	struct Library *timerBase;

	U16 board;
	U16 options;
	U16 word;
	U8 address[6];
	U32 index;
	U8 nonzero;
	S32 error;

	control = unit->unit_USB_Register->Res_Control;
	timer = NULL;
	word = 0;
	index = 0;
	nonzero = 0;
	unit->unit_BoardOptions = 0xffff;

	error = SiS163U_ReadEEPROM( control, NULL, 0, & word );

	if ( ( error == USB2Err_Host_Timeout ) || ( ( ! error ) && ( word != SIS163U_EEPROM_SIGNATURE ) ) )
	{
		USBINFO( "SiS163U: hardware EEPROM signature $%04lx, USB error %ld, trying serial read", (U32) word, error );

		// The USB stack already keeps timer.device open.
		timerBase = (PTR) FindName( & ( (struct ExecBase *) SysBase )->DeviceList, "timer.device" );

		if ( timerBase )
		{
			timer = (PTR) GetInterface( timerBase, "main", 1, NULL );
		}

		if ( timer )
		{
			error = SiS163U_ReadEEPROM( control, timer, 0, & word );
		}
	}

	if ( ( error ) || ( word != SIS163U_EEPROM_SIGNATURE ) )
	{
		USBERROR( "SiS163U: EEPROM signature $%04lx, USB error %ld", (U32) word, error );

		goto bailout;
	}

	for( index = 0; index < sizeof( address ); index += 2 )
	{
		error = SiS163U_ReadEEPROM( control, timer, SIS163U_EEPROM_MAC_WORD + index / 2, & word );

		if ( error )
		{
			USBERROR( "SiS163U: EEPROM station address read failed at word %lu, USB error %ld",
				SIS163U_EEPROM_MAC_WORD + index / 2, error );

			break;
		}

		address[index] = word;
		address[index + 1] = word >> 8;
		nonzero |= address[index] | address[index + 1];
	}

	if ( index == sizeof( address ) )
	{
		error = SiS163U_ReadEEPROM( control, timer, SIS163U_EEPROM_RF_WORD, & word );

		if ( error )
		{
			USBERROR( "SiS163U: EEPROM RF type read failed (USB error %ld)", error );
		}
		else
		{
			unit->unit_RFType = word;

			USBINFO( "SiS163U: EEPROM RF type $%02lx (word $%04lx)", (U32) ( word & 0xff ), (U32) word );
		}

		if ( unit->unit_RFType == 0x08 )
		{
			error = SiS163U_ReadEEPROM( control, timer, SIS163U_EEPROM_BOARD_WORD, & board );

			if ( ! error )
			{
				unit->unit_BoardOptions = board;

				error = SiS163U_ReadEEPROM( control, timer, SIS163U_EEPROM_OPTIONS_WORD, & options );
			}

			if ( error )
			{
				USBERROR( "SiS163U: EEPROM board options read failed, USB error %ld", error );
			}
			else
			{
				USBINFO( "SiS163U: EEPROM board options, word $18 = $%04lx, word $21 = $%04lx",
					(U32) board, (U32) options );
			}
		}
	}

bailout:
	if ( timer )
	{
		DropInterface( (PTR) timer );
	}

	if ( index != sizeof( address ) )
	{
		return ( FALSE );
	}

	if ( ( ! nonzero ) || ( address[0] & 1 ) )
	{
		USBERROR( "SiS163U: EEPROM contains an invalid station address" );

		return ( FALSE );
	}

	memcpy( unit->unit_StationAddress, address, sizeof( address ) );

	return ( TRUE );
}

// --
