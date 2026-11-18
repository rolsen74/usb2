
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
myReadChipID( struct devUnit *unit )
{
uint16 val;
int retval;

	retval = FALSE;

	if ( myRead2( unit, URE_PLA_TCR1, URE_MCU_TYPE_PLA, & val ))
	{
		SHOWMSG( "Read Error" );
		goto bailout;
	}

	// I guess 0x4xxx is 8152 and 0x5xxx is 8153

	switch( val & URE_VERSION_MASK )
	{
		case 0x4c00:
		{
			unit->unit_ChipVersion = URE_CHIP_VER_4C00;
			unit->unit_Support_GMII = FALSE;

			SHOWMSG( "Chip Version: %04x", val & URE_VERSION_MASK );
			break;
		}

		case 0x4c10:
		{
			unit->unit_ChipVersion = URE_CHIP_VER_4C10;
			unit->unit_Support_GMII = FALSE;

			SHOWMSG( "Chip Version: %04lx", val & URE_VERSION_MASK );
			break;
		}

		case 0x5c00:
		{
			unit->unit_ChipVersion = URE_CHIP_VER_5C00;
			unit->unit_Support_GMII = TRUE;

			SHOWMSG( "Chip Version: %04lx", val & URE_VERSION_MASK );
			break;
		}

		case 0x5c10:
		{
			unit->unit_ChipVersion = URE_CHIP_VER_5C10;
			unit->unit_Support_GMII = TRUE;

			SHOWMSG( "Chip Version: %04lx", val & URE_VERSION_MASK );
			break;
		}

		case 0x5c20:
		{
			unit->unit_ChipVersion = URE_CHIP_VER_5C20;
			unit->unit_Support_GMII = TRUE;

			SHOWMSG( "Chip Version: %04lx", val & URE_VERSION_MASK );
			break;
		}

		case 0x5c30:
		{
			unit->unit_ChipVersion = URE_CHIP_VER_5C30;
			unit->unit_Support_GMII = TRUE;

			SHOWMSG( "Chip Version: %04lx", val & URE_VERSION_MASK );
			break;
		}

		default:
		{
			IExec->DebugPrintF( "Unknown Chip Version (0x%02lx)\n", val & URE_VERSION_MASK );
			goto bailout;
		}
	}

	retval = TRUE;

bailout:
	return(		retval );
}

// --
