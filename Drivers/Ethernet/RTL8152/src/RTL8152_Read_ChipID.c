
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
S32 error;
U32 attempt;

	retval = FALSE;

	error = myRead2( unit, URE_PLA_TCR1, URE_MCU_TYPE_PLA, & val );

	// Only retry this read-only probe before any chip registers are written.
	for( attempt = 0; ( attempt < 2 ) && ( error == USB2Err_Host_Timeout ); attempt++ )
	{
		IDOS->Delay( 5 );

		unit->unit_USBControlError = USB2Err_NoError;

		error = myRead2( unit, URE_PLA_TCR1, URE_MCU_TYPE_PLA, & val );
	}

	if ( error )
	{
		if ( error == USB2Err_Host_Timeout )
		{
			RTL8152_ReportUSBConfiguration( unit );
		}

		goto bailout;
	}

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

		case 0x6000:
		case 0x6010:
		{
			unit->unit_ChipVersion = ( val & URE_VERSION_MASK ) == 0x6000 ?
				URE_CHIP_VER_6000 : URE_CHIP_VER_6010;
			unit->unit_Support_GMII = TRUE;
			break;
		}

		default:
		{
			IExec->DebugPrintF( "RTL8152: unsupported chip revision $%04lx\n", (U32) ( val & URE_VERSION_MASK ) );
			goto bailout;
		}
	}

	IExec->DebugPrintF( "RTL8152: %s chip revision $%04lx\n",
		RTL8153B_CHIP( unit ) ? "RTL8153B" : ( unit->unit_Support_GMII ? "RTL8153" : "RTL8152" ),
		(U32) ( val & URE_VERSION_MASK ) );

	retval = TRUE;

bailout:
	return(		retval );
}

// --
