
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Pegasus.h"

// --

S32
Pegasus_Read( struct devUnit *unit, U8 reg, U8 *data, U16 length )
{
	return ( ! Wired_ControlRequest( unit, REQTYPE_Read | REQTYPE_Vendor | REQTYPE_Device,
		PEG_READ_REGS, 0, reg, data, length ) );
}

// --

S32
Pegasus_Write( struct devUnit *unit, U8 reg, U8 *data, U16 length )
{
	return ( ! Wired_ControlRequest( unit, REQTYPE_Write | REQTYPE_Vendor | REQTYPE_Device,
		PEG_WRITE_REGS, 0, reg, data, length ) );
}

// --

S32
Pegasus_WriteByte( struct devUnit *unit, U8 reg, U8 value )
{
	// A single-register write puts the value in wValue as well as the data stage.
	return ( ! Wired_ControlRequest( unit, REQTYPE_Write | REQTYPE_Vendor | REQTYPE_Device,
		PEG_WRITE_REGS, value, reg, & value, 1 ) );
}

// --

S32
Pegasus_WaitRegister( struct devUnit *unit, U8 reg, U8 mask, U8 expected )
{
	U32 retry;
	U8 value;

	for( retry = 0; retry < 20; retry++ )
	{
		if ( ! Pegasus_Read( unit, reg, & value, 1 ) )
		{
			return ( FALSE );
		}

		if ( ( value & mask ) == expected )
		{
			return ( TRUE );
		}

		IDOS->Delay( 1 );
	}

	return ( FALSE );
}

// --

static S32
ProgramStation( struct devUnit *unit, const U8 *address )
{
	U8 mac[6];

	memcpy( mac, address, sizeof( mac ) );

	return ( ( Pegasus_Write( unit, PEG_ETH_ID, mac, sizeof( mac ) ) ) &&
		( Pegasus_Read( unit, PEG_ETH_ID, mac, sizeof( mac ) ) ) &&
		( ! memcmp( mac, address, sizeof( mac ) ) ) );
}

// --

S32
Wired_SetMacAddress( struct devUnit *unit, const U8 *address )
{
	if ( ! ProgramStation( unit, address ) )
	{
		IExec->DebugPrintF( "Pegasus: station address write/readback failed\n" );

		if ( ( unit->unit_Configured ) && ( ! ProgramStation( unit, unit->unit_CurrentAddress ) ) )
		{
			unit->unit_ControlNeedsSetup = TRUE;

			IExec->DebugPrintF( "Pegasus: could not restore station address, hardware setup required\n" );
		}

		return ( FALSE );
	}

	if ( unit->unit_Running )
	{
		IExec->DebugPrintF( "Pegasus: SANA-II configured station %02lx:%02lx:%02lx:%02lx:%02lx:%02lx\n",
			(U32) address[0], (U32) address[1], (U32) address[2],
			(U32) address[3], (U32) address[4], (U32) address[5] );
	}

	return ( TRUE );
}

// --
