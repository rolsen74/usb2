
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

static S32
StartPHYAccess( struct devUnit *unit, U8 phy, U8 reg, U8 command, U16 value )
{
	U8 window[4];

	window[0] = phy;
	window[1] = value;
	window[2] = value >> 8;
	window[3] = reg;

	return ( ( Pegasus_WriteByte( unit, PEG_PHY_CTRL, 0 ) ) &&
		( Pegasus_Write( unit, PEG_PHY_ADDR, window, sizeof( window ) ) ) &&
		( Pegasus_WriteByte( unit, PEG_PHY_CTRL, reg | command ) ) &&
		( Pegasus_WaitRegister( unit, PEG_PHY_CTRL, PEG_PHY_DONE, PEG_PHY_DONE ) ) );
}

// --

S32
Pegasus_ReadPHY( struct devUnit *unit, U8 phy, U8 reg, U16 *value )
{
	U8 data[2];

	if ( ( ! StartPHYAccess( unit, phy, reg, PEG_PHY_READ, 0 ) ) ||
		( ! Pegasus_Read( unit, PEG_PHY_DATA, data, sizeof( data ) ) ) )
	{
		return ( FALSE );
	}

	*value = data[0] | ( (U16) data[1] << 8 );

	return ( TRUE );
}

// --

S32
Pegasus_WritePHY( struct devUnit *unit, U8 phy, U8 reg, U16 value )
{
	return ( StartPHYAccess( unit, phy, reg, PEG_PHY_WRITE, value ) );
}

// --
