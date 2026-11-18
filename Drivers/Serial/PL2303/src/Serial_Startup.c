/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE S32 Serial_Startup( struct SerialContext *ctx, struct SerialUnit *unit )
{
U8 data;
S32 result;

	result = TRUE;

	result &= Serial_Vendor_Read( ctx, unit, 0x8484, & data );
	result &= Serial_Vendor_Write( ctx, unit, 0x8484, 0 );
	result &= Serial_Vendor_Read( ctx, unit, 0x8484, & data );
	result &= Serial_Vendor_Read( ctx, unit, 0x8383, & data );
	result &= Serial_Vendor_Read( ctx, unit, 0x8484, & data );
	result &= Serial_Vendor_Write( ctx, unit, 0x8484, 1 );
	result &= Serial_Vendor_Read( ctx, unit, 0x8484, & data );
	result &= Serial_Vendor_Read( ctx, unit, 0x8383, & data );
	result &= Serial_Vendor_Write( ctx, unit, 0, 1 );
	result &= Serial_Vendor_Write( ctx, unit, 1, 0 );
	result &= Serial_Vendor_Write( ctx, unit, 2, 0x44 );

	if ( ! result )
	{
		return ( FALSE );
	}

	// CDC line coding: 9600 baud, one stop bit, no parity, eight data bits.
	unit->Line[0] = 0x80;
	unit->Line[1] = 0x25;
	unit->Line[2] = 0;
	unit->Line[3] = 0;
	unit->Line[4] = 0;
	unit->Line[5] = 0;
	unit->Line[6] = 8;

	result = Serial_Control( ctx, unit, REQTYPE_Write | REQTYPE_Class | REQTYPE_Interface,
		0x20, 0, unit->StartMsg->InterfaceDescriptor->InterfaceNumber, unit->Line, 7 );

	if ( result )
	{
		result = Serial_Control( ctx, unit, REQTYPE_Write | REQTYPE_Class | REQTYPE_Interface,
			0x22, 3, unit->StartMsg->InterfaceDescriptor->InterfaceNumber, NULL, 0 );
	}

	return ( result );
}

// --
