
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
	S32 error;
	S32 restore;

	unit->unit_USBControlError = 0;

	error = myWrite1( unit, URE_PLA_CRWECR, URE_MCU_TYPE_PLA, URE_CRWECR_CONFIG );

	if ( ! error )
	{
		error = myWrite6( unit, URE_PLA_IDR, URE_MCU_TYPE_PLA, address );
	}

	// Leave configuration mode even when the address write fails.
	unit->unit_USBControlError = 0;

	restore = myWrite1( unit, URE_PLA_CRWECR, URE_MCU_TYPE_PLA, URE_CRWECR_NORAML );

	unit->unit_ControlNeedsSetup = restore != 0;

	return ( ! error && ! restore );
}

// --
