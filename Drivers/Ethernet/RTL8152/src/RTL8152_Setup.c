
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Controller.h"

// --

int
Driver_Setup( struct devUnit *unit )
{
	U16 filter;
	U32 accept;
	U8 command;

	if ( ( ! myReadChipID( unit ) ) || ( unit->unit_Support_GMII ) ||
		( ! RTL8152_GetMacAddress( unit ) ) )
	{
		return ( FALSE );
	}

	RTL8152_Init( unit );

	if ( ! RTL8152_Up( unit ) )
	{
		return ( FALSE );
	}

	if ( ! RTL8152_SetMacAddress( unit, unit->unit_CurrentAddress ) )
	{
		return ( FALSE );
	}

	if ( ! RTL8152_SetSpeed( unit, TRUE, SPEED_100, TRUE ) )
	{
		return ( FALSE );
	}

	myRead2( unit, URE_PLA_FMC, URE_MCU_TYPE_PLA, & filter );

	myWrite2( unit, URE_PLA_FMC, URE_MCU_TYPE_PLA, filter & ~URE_FMC_FCR_MCU_EN );

	myWrite2( unit, URE_PLA_FMC, URE_MCU_TYPE_PLA, filter | URE_FMC_FCR_MCU_EN );

	myRead1( unit, URE_PLA_CR, URE_MCU_TYPE_PLA, & command );

	myWrite1( unit, URE_PLA_CR, URE_MCU_TYPE_PLA, command | URE_CR_RE | URE_CR_TE );

	rxdy_gated_en( unit, FALSE );

	myRead4( unit, URE_PLA_RCR, URE_MCU_TYPE_PLA, & accept );

	// Accept all hardware hash matches and filter exact addresses in ReadPacket.
	myWrite4( unit, URE_PLA_MAR0, URE_MCU_TYPE_PLA, 0xffffffffU );

	myWrite4( unit, URE_PLA_MAR4, URE_MCU_TYPE_PLA, 0xffffffffU );

	myWrite4( unit, URE_PLA_RCR, URE_MCU_TYPE_PLA, accept | URE_RCR_APM | URE_RCR_AM | URE_RCR_AB );

	return ( ! unit->unit_USBControlError );
}

// --
