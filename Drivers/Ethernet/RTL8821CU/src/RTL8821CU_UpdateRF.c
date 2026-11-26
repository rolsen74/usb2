
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Radio.h"
#include "Registers.h"

#include <interfaces/timer.h>

// --

#define RF_PATH_A_READ 0x2800
#define RF_PATH_A_WRITE 0x0c90
#define RF_DATA_MASK 0x000fffff

S32
RTL8821CU_UpdateRF( struct EthernetUnit *unit, U8 address, U32 mask, U32 value )
{
	struct USB2_EPResource *control;

	U32 current;
	U32 command;

	control = unit->unit_USB_Register->Res_Control;
	mask &= RF_DATA_MASK;
	current = 0;

	// The direct window reads RF registers. Writes go through the serial port.
	if ( ( mask != RF_DATA_MASK ) &&
		( ! RTL8821CU_ReadReg( control, RF_PATH_A_READ + ( (U16) address << 2 ), 4, & current ) ) )
	{
		return ( FALSE );
	}

	command = ( (U32) address << 20 ) | ( current & ( RF_DATA_MASK & ~mask ) ) | ( value & mask );

	if ( ! RTL8821CU_WriteReg( control, RF_PATH_A_WRITE, 4, command ) )
	{
		return ( FALSE );
	}

	unit->unit_Timer->MicroDelay( 13 );

	return ( TRUE );
}

// --
