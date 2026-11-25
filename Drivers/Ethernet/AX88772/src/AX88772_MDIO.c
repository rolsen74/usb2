
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "AX88772.h"

// --

S32
AX88772_StartMDIO( struct devUnit *unit )
{
	U32 attempt;
	U8 status;

	if ( AX88772_Write( unit, AX_SW_MII, 0, 0 ) )
	{
		return ( FALSE );
	}

	for( attempt = 0; attempt < 5; attempt++ )
	{
		if ( AX88772_Read( unit, AX_MII_STATUS, 0, 0, & status, sizeof( status ) ) )
		{
			break;
		}

		if ( status & AX_HOST_ENABLE )
		{
			return ( TRUE );
		}

		IDOS->Delay( 1 );
	}

	AX88772_EndMDIO( unit );

	return ( FALSE );
}

// --

S32
AX88772_EndMDIO( struct devUnit *unit )
{
	return ( ! AX88772_Write( unit, AX_HW_MII, 0, 0 ) );
}

// --

S32
AX88772_ReadPHY( struct devUnit *unit, U16 reg, U16 *value )
{
	U8 bytes[2];

	if ( AX88772_Read( unit, AX_READ_MII, unit->unit_AXPHY, reg, bytes, sizeof( bytes ) ) )
	{
		return ( FALSE );
	}

	*value = bytes[0] | ( (U16) bytes[1] << 8 );

	return ( TRUE );
}

// --

S32
AX88772_WritePHY( struct devUnit *unit, U16 reg, U16 value )
{
	U8 bytes[2];

	bytes[0] = value;
	bytes[1] = value >> 8;

	return ( ! Wired_ControlRequest( unit, REQTYPE_Write | REQTYPE_Vendor | REQTYPE_Device,
		AX_WRITE_MII, unit->unit_AXPHY, reg, bytes, sizeof( bytes ) ) );
}

// --
