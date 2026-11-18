
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
RTL8152_GetMacAddress( struct devUnit *unit )
{
uint8 eaddr[8];
int retval;

	retval = FALSE;

	// -- Read Mac Address

	if ( unit->unit_ChipVersion == URE_CHIP_VER_4C00 )
    {
		if ( myReadMem( unit, URE_PLA_IDR, URE_MCU_TYPE_PLA, eaddr, 8 ))
		{
			SHOWMSG( "Read Error\n" );
			goto bailout;
		}
    }
    else
    {
		if ( myReadMem( unit, URE_PLA_BACKUP, URE_MCU_TYPE_PLA, eaddr, 8 ))
		{
			SHOWMSG( "Read Error\n" );
			goto bailout;
		}
    }

	SHOWMSG( "MAC Address: %02lx:%02lx:%02lx:%02lx:%02lx:%02lx",
		eaddr[0], eaddr[1], eaddr[2], eaddr[3], eaddr[4], eaddr[5]
	);

	memcpy( unit->unit_DefaultAddress, eaddr, 6 );
	memcpy( unit->unit_CurrentAddress, eaddr, 6 );

	retval = TRUE;

bailout:
	return(		retval );
}

// --
