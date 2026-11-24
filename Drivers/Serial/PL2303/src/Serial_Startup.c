/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE static enum SerialPLType
Serial_PLType( struct SerialContext *ctx, struct SerialUnit *unit )
{
	struct USB2_Device_Desc desc;

	U8 status;

	if ( USB2_Attr_Get( unit->StartMsg->Interface->FKT_ID, USB2Tag_Attr_DeviceDesc, &desc, sizeof( desc ) ) != sizeof( desc ) )
	{
		return ( SERIAL_PL_HX );
	}

	if ( ( desc.DeviceClass == 2 ) || ( desc.MaxPacketSize0 != 64 ) )
	{
		return ( SERIAL_PL_H );
	}

	if ( LE_SWAP16( desc.USBVers ) == 0x0200 )
	{
		switch( LE_SWAP16( desc.DeviceVers ) )
		{
			case 0x0300:
			case 0x0500:
				// TA and TB still answer the HX status request.
				if ( Serial_Vendor_Read( ctx, unit, 0x8080, &status ) )
				{
					return ( SERIAL_PL_HX );
				}
				/* fall through */
			case 0x0100:
			case 0x0105:
			case 0x0305:
			case 0x0400:
			case 0x0405:
			case 0x0505:
			case 0x0600:
			case 0x0605:
			case 0x0700:
			case 0x0705:
			case 0x0905:
			case 0x1005:
				return ( SERIAL_PL_HXN );
		}
	}

	return ( SERIAL_PL_HX );
}

// --

SEC_CODE S32
Serial_DefaultLine( struct SerialContext *ctx, struct SerialUnit *unit )
{
	U8 line[7];

	line[0] = 0x80;
	line[1] = 0x25;
	line[2] = 0;
	line[3] = 0;
	line[4] = 0;
	line[5] = 0;
	line[6] = 8;

	if ( ( ( ! unit->ACM ) || ( unit->ACMCapabilities & SERIAL_ACM_LINE ) ) &&
		( ! Serial_Control( ctx, unit, REQTYPE_Write | REQTYPE_Class | REQTYPE_Interface,
			0x20, 0, unit->ACM ? unit->StartMsg->InterfaceDescriptor->InterfaceNumber : 0, line, sizeof( line ) ) ) )
	{
		return ( FALSE );
	}

	CopyMem( line, unit->Line, sizeof( line ) );

	return ( TRUE );
}

// --

SEC_CODE S32
Serial_Startup( struct SerialContext *ctx, struct SerialUnit *unit )
{
	U8 data;
	S32 result;

	if ( ! unit->ACM )
	{
		unit->PLType = Serial_PLType( ctx, unit );
	}

	result = TRUE;

	if ( ( ! unit->ACM ) && ( unit->PLType != SERIAL_PL_HXN ) )
	{
		result = result && Serial_Vendor_Read( ctx, unit, 0x8484, & data );
		result = result && Serial_Vendor_Write( ctx, unit, 0x0404, 0 );
		result = result && Serial_Vendor_Read( ctx, unit, 0x8484, & data );
		result = result && Serial_Vendor_Read( ctx, unit, 0x8383, & data );
		result = result && Serial_Vendor_Read( ctx, unit, 0x8484, & data );
		result = result && Serial_Vendor_Write( ctx, unit, 0x0404, 1 );
		result = result && Serial_Vendor_Read( ctx, unit, 0x8484, & data );
		result = result && Serial_Vendor_Read( ctx, unit, 0x8383, & data );
		result = result && Serial_Vendor_Write( ctx, unit, 0, 1 );
		result = result && Serial_Vendor_Write( ctx, unit, 1, 0 );
		result = result && Serial_Vendor_Write( ctx, unit, 2, ( unit->PLType == SERIAL_PL_H ) ? 0x24 : 0x44 );

		if ( ! result )
		{
			return ( FALSE );
		}
	}

	if ( ( ! unit->ACM ) && ( unit->PLType == SERIAL_PL_HXN ) &&
		( ! Serial_Control( ctx, unit, REQTYPE_Write | REQTYPE_Vendor | REQTYPE_Device,
			0x80, 7, 3, NULL, 0 ) ) )
	{
		return ( FALSE );
	}

	if ( ! Serial_DefaultLine( ctx, unit ) )
	{
		return ( FALSE );
	}

	if ( ( unit->ACM ) && ( ! ( unit->ACMCapabilities & SERIAL_ACM_LINE ) ) )
	{
		return ( TRUE );
	}

	return ( Serial_Control( ctx, unit, REQTYPE_Write | REQTYPE_Class | REQTYPE_Interface,
		0x22, 3, unit->ACM ? unit->StartMsg->InterfaceDescriptor->InterfaceNumber : 0, NULL, 0 ) );
}

// --
