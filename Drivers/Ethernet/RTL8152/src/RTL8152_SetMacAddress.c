
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

S32
RTL8152_SetMacAddress( struct devUnit *unit, const U8 *address )
{
	// is this only for URE_CHIP_VER_4C00 ??

	myWrite1( unit, URE_PLA_CRWECR, URE_MCU_TYPE_PLA, URE_CRWECR_CONFIG );
	
	myWrite6( unit, URE_PLA_IDR,	URE_MCU_TYPE_PLA, address );

	myWrite1( unit, URE_PLA_CRWECR, URE_MCU_TYPE_PLA, URE_CRWECR_NORAML );

	return ( ! unit->unit_USBControlError );

}

// --
