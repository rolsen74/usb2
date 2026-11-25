
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

// Register sequences and board-specific PHY settings follow the Linux Pegasus driver.
// https://github.com/torvalds/linux/blob/master/drivers/net/usb/pegasus.c

#define PEG_VENDOR_BELKIN  0x050d
#define PEG_VENDOR_LINKSYS 0x066b
#define PEG_VENDOR_ELCON   0x0db7
#define PEG_VENDOR_DLINK   0x2001
#define PEG_BELKIN_F5D5050 0x0121

#define PEG_PHY_AUX_MODE   0x1b
#define PEG_PHY_LED_MODE   0x0004

// --

static S32
SetBoardPHYMode( struct devUnit *unit, U8 phy )
{
	U16 mode;

	return ( ( Pegasus_ReadPHY( unit, phy, PEG_PHY_AUX_MODE, & mode ) ) &&
		( Pegasus_WritePHY( unit, phy, PEG_PHY_AUX_MODE, mode | PEG_PHY_LED_MODE ) ) );
}

// --

static S32
ResetMAC( struct devUnit *unit, S32 repeatGPIO )
{
	if ( ( ! unit->unit_USBBulkOut->EndPointMax ) ||
		( ! Pegasus_WriteByte( unit, PEG_ETH_CTRL1, PEG_MAC_RESET ) ) ||
		( ! Pegasus_WaitRegister( unit, PEG_ETH_CTRL1, PEG_MAC_RESET, 0 ) ) ||
		( ! Pegasus_WriteByte( unit, PEG_GPIO1, 0x26 ) ) ||
		( ! Pegasus_WriteByte( unit, PEG_GPIO0, 0x24 ) ) ||
		( ! Pegasus_WriteByte( unit, PEG_GPIO0, 0x26 ) ) )
	{
		return ( FALSE );
	}

	// Linksys and D-Link boards need a second GPIO transition.
	return ( ( ! repeatGPIO ) || ( ( Pegasus_WriteByte( unit, PEG_GPIO0, 0x24 ) ) &&
		( Pegasus_WriteByte( unit, PEG_GPIO0, 0x26 ) ) ) );
}

// --

static S32
ReadStationAddress( struct devUnit *unit, U8 *address )
{
	static const U8 zero[6] = { 0 };

	U8 word;

	if ( ( Pegasus_Read( unit, PEG_ETH_ID, address, 6 ) ) &&
		( ! IS_MULTICAST( address ) ) && ( memcmp( address, zero, sizeof( zero ) ) ) )
	{
		return ( TRUE );
	}

	for( word = 0; word < 3; word++ )
	{
		if ( ( ! Pegasus_WriteByte( unit, PEG_EEPROM_CTRL, 0 ) ) ||
			( ! Pegasus_WriteByte( unit, PEG_EEPROM_OFFSET, word ) ) ||
			( ! Pegasus_WriteByte( unit, PEG_EEPROM_CTRL, PEG_EEPROM_READ ) ) ||
			( ! Pegasus_WaitRegister( unit, PEG_EEPROM_CTRL, PEG_EEPROM_DONE, PEG_EEPROM_DONE ) ) ||
			( ! Pegasus_Read( unit, PEG_EEPROM_DATA, address + 2 * word, 2 ) ) )
		{
			return ( FALSE );
		}
	}

	return ( ( ! IS_MULTICAST( address ) ) && ( memcmp( address, zero, sizeof( zero ) ) ) );
}

// --

static S32
SetupPHY( struct devUnit *unit )
{
	U8 value;

	// Select the Ethernet PHY after its power-up interval.
	if ( ( ! Pegasus_WriteByte( unit, PEG_RX_OPTIONS, 0 ) ) ||
		( ! Pegasus_WriteByte( unit, PEG_PHY_SELECT, 1 ) ) )
	{
		return ( FALSE );
	}

	IDOS->Delay( 5 );

	if ( ( ! Pegasus_WriteByte( unit, PEG_PHY_SELECT, 2 ) ) ||
		( ! Pegasus_WriteByte( unit, 0x83, 0xa5 ) ) ||
		( ! Pegasus_Read( unit, 0x83, & value, 1 ) ) )
	{
		return ( FALSE );
	}

	// The ADM8513-compatible register window indicates length-prefixed RX.
	unit->unit_PegasusRXPrefix = ( value == 0xa5 );

	return ( ( Pegasus_WriteByte( unit, 0x80, 0xc0 ) ) &&
		( Pegasus_WriteByte( unit, 0x83, 0xff ) ) &&
		( Pegasus_WriteByte( unit, 0x84, 1 ) ) &&
		( Pegasus_WriteByte( unit, 0x81, 2 ) ) );
}

// --

static S32
FindPHY( struct devUnit *unit )
{
	U16 status;
	U8 phy;

	for( phy = 0; phy < 32; phy++ )
	{
		if ( ! Pegasus_ReadPHY( unit, phy, PEG_MII_STATUS, & status ) )
		{
			return ( FALSE );
		}

		if ( ( status != 0xffff ) && ( status & PEG_MII_MEDIA ) )
		{
			unit->unit_PegasusPHY = phy;

			return ( TRUE );
		}
	}

	return ( FALSE );
}

// --

S32
Driver_Setup( struct devUnit *unit )
{
	struct USB2_Device_Desc descriptor;
	struct USB2IFace *IUSB2;

	U16 vendor;
	S32 linksysOrDLink;
	U8 address[6];

	IUSB2 = unit->unit_IUSB2;

	if ( IUSB2->USB2_Attr_Get( unit->unit_USBInterface->FKT_ID, USB2Tag_Attr_DeviceDesc,
		& descriptor, sizeof( descriptor ) ) != sizeof( descriptor ) )
	{
		return ( FALSE );
	}

	vendor = LE_SWAP16( descriptor.VendorID );
	linksysOrDLink = ( ( vendor == PEG_VENDOR_LINKSYS ) || ( vendor == PEG_VENDOR_DLINK ) );

	// Belkin also used this VID:PID on a Bluetooth dongle.
	if ( ( vendor == PEG_VENDOR_BELKIN ) && ( LE_SWAP16( descriptor.DeviceID ) == PEG_BELKIN_F5D5050 ) &&
		( descriptor.DeviceClass != 0 ) )
	{
		return ( FALSE );
	}

	if ( ! ResetMAC( unit, linksysOrDLink ) )
	{
		IExec->DebugPrintF( "Pegasus: MAC reset failed\n" );

		return ( FALSE );
	}

	if ( ( vendor == PEG_VENDOR_ELCON ) && ( ! SetBoardPHYMode( unit, 3 ) ) )
	{
		IExec->DebugPrintF( "Pegasus: could not configure ELCON PHY\n" );

		return ( FALSE );
	}

	if ( ! ReadStationAddress( unit, address ) )
	{
		IExec->DebugPrintF( "Pegasus: could not read station address\n" );

		return ( FALSE );
	}

	if ( ( ! SetupPHY( unit ) ) || ( ! FindPHY( unit ) ) )
	{
		IExec->DebugPrintF( "Pegasus: could not initialize or find the Ethernet PHY\n" );

		return ( FALSE );
	}

	if ( ( linksysOrDLink ) && ( ! SetBoardPHYMode( unit, 0 ) ) )
	{
		IExec->DebugPrintF( "Pegasus: could not configure Linksys/D-Link PHY LEDs\n" );

		return ( FALSE );
	}

	if ( ( ! Wired_SetMacAddress( unit, address ) ) ||
		( ! Pegasus_WriteByte( unit, PEG_WAKE_CTRL, 0x04 ) ) ||
		( ! Pegasus_WriteByte( unit, PEG_ETH_CTRL0, 0 ) ) )
	{
		IExec->DebugPrintF( "Pegasus: MAC configuration failed\n" );

		return ( FALSE );
	}

	memcpy( unit->unit_DefaultAddress, address, sizeof( address ) );

	memcpy( unit->unit_CurrentAddress, address, sizeof( address ) );

	unit->unit_PegasusMACState = PEG_MAC_STOPPED;
	unit->unit_PegasusStartFailures = 0;
	unit->unit_PegasusStopFailures = 0;
	unit->unit_ControlNeedsSetup = FALSE;

	IExec->DebugPrintF( "Pegasus: PHY %lu, %s RX, station %02lx:%02lx:%02lx:%02lx:%02lx:%02lx\n",
		(U32) unit->unit_PegasusPHY, unit->unit_PegasusRXPrefix ? "length-prefixed" : "trailing status",
		(U32) address[0], (U32) address[1], (U32) address[2], (U32) address[3], (U32) address[4], (U32) address[5] );

	return ( TRUE );
}

// --
