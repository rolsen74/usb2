
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
AX88772_Read( struct devUnit *unit, U8 command, U16 value, U16 index, U8 *data, U16 size )
{
	return ( Wired_ControlRequest( unit, REQTYPE_Read | REQTYPE_Vendor | REQTYPE_Device,
		command, value, index, data, size ) );
}

// --

S32
AX88772_Write( struct devUnit *unit, U8 command, U16 value, U16 index )
{
	return ( Wired_ControlRequest( unit, REQTYPE_Write | REQTYPE_Vendor | REQTYPE_Device,
		command, value, index, NULL, 0 ) );
}

// --

S32
Wired_SetMacAddress( struct devUnit *unit, const U8 *address )
{
	U8 mac[6];

	memcpy( mac, address, sizeof( mac ) );

	return ( ! Wired_ControlRequest( unit, REQTYPE_Write | REQTYPE_Vendor | REQTYPE_Device,
		AX_WRITE_MAC, 0, 0, mac, sizeof( mac ) ) );
}

// --
