
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

// Ralink USB request codes and register layout match Linux rt2800usb
// and FreeBSD run(4). EEPROM request indices are byte offsets.
#define RT2870_ASIC_VERSION		0x1000
#define RT2870_EEPROM_MAC		0x0004
#define RT2870_READY_ATTEMPTS	50
#define RT2860D_ASIC_VERSION	0x28600101

// --

S32
RT2870_Probe( struct EthernetUnit *unit, const struct USB2_DriverMessage *msg )
{
	struct USB2_EPResource *control;

	U32 version;
	U32 attempt;
	U32 offset;
	U8 *address;
	U8 addressBits;
	S32 error;

	control = unit->unit_USB_Register->Res_Control;
	address = unit->unit_StationAddress;
	addressBits = 0;

	// The ASIC may report zero or all ones while coming out of reset.
	for( attempt = 0; attempt < RT2870_READY_ATTEMPTS; attempt++ )
	{
		error = RT2870_ReadRegister( control, RT2870_ASIC_VERSION, & version );

		if ( error )
		{
			USBERROR( "RT2870: ASIC version read failed, USB error %ld", error );

			return ( FALSE );
		}

		if ( ( version != 0 ) && ( version != 0xffffffff ) )
		{
			break;
		}

		Delay( 1 );
	}

	if ( attempt == RT2870_READY_ATTEMPTS )
	{
		USBERROR( "RT2870: ASIC did not become ready" );

		return ( FALSE );
	}

	USBINFO( "RT2870: %04lX:%04lX MAC/BBP $%04lx revision $%04lx",
		(U32) msg->VendorID, (U32) msg->DeviceID, version >> 16, version & 0xffff );

	if ( version != RT2860D_ASIC_VERSION )
	{
		USBERROR( "RT2870: unsupported ASIC $%08lx, expected $%08lx", version, (U32) RT2860D_ASIC_VERSION );

		return ( FALSE );
	}

	for( offset = 0; offset < sizeof( unit->unit_StationAddress ); offset += 2 )
	{
		error = RT2870_Control( control, REQTYPE_Read, RT2870_READ_EEPROM, 0,
			RT2870_EEPROM_MAC + offset, & address[offset], 2 );

		if ( error )
		{
			USBERROR( "RT2870: EEPROM read at $%04lx failed, USB error %ld", RT2870_EEPROM_MAC + offset, error );

			return ( FALSE );
		}

		addressBits |= address[offset] | address[offset + 1];
	}

	if ( ( ! addressBits ) || ( address[0] & 1 ) )
	{
		USBERROR( "RT2870: EEPROM has no valid station address" );

		return ( FALSE );
	}

	USBINFO( "RT2870: EEPROM station address %02lx:%02lx:%02lx:%02lx:%02lx:%02lx",
		(U32) address[0], (U32) address[1], (U32) address[2],
		(U32) address[3], (U32) address[4], (U32) address[5] );

	if ( RT2870_LoadFirmware( control, version >> 16 ) )
	{
		unit->unit_RxCheckReady = RT2870_InitBaseband( control );
	}

	return ( unit->unit_RxCheckReady );
}

// --
