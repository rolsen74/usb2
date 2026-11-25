
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

static S32
ReadStation( struct devUnit *unit, U8 chip )
{
	static const U8 empty[6] = { 0 };

	U8 address[6];
	U32 index;

	if ( unit->unit_AXMACValid )
	{
		return ( TRUE );
	}

	if ( ( chip == AX_CHIP_772B ) || ( AX88772_Read( unit, AX_READ_MAC, 0, 0, address, sizeof( address ) ) ) ||
		( IS_MULTICAST( address ) ) || ( ! memcmp( address, empty, sizeof( address ) ) ) )
	{
		for( index = 0; index < 3; index++ )
		{
			if ( AX88772_Read( unit, AX_READ_EEPROM, 4 + index, 0, address + 2 * index, 2 ) )
			{
				return ( FALSE );
			}
		}

		if ( ( IS_MULTICAST( address ) ) || ( ! memcmp( address, empty, sizeof( address ) ) ) )
		{
			return ( FALSE );
		}
	}

	memcpy( unit->unit_DefaultAddress, address, sizeof( address ) );

	memcpy( unit->unit_CurrentAddress, address, sizeof( address ) );

	unit->unit_AXMACValid = TRUE;

	return ( TRUE );
}

// --

static S32
ResetPHY( struct devUnit *unit )
{
	U8 embedded;

	embedded = ( unit->unit_AXPHY == AX_PHY_INTERNAL );

	if ( AX88772_Write( unit, AX_SELECT_PHY, embedded, 0 ) )
	{
		return ( FALSE );
	}

	if ( embedded )
	{
		if ( AX88772_Write( unit, AX_RESET, AX_POWER_DOWN_PHY, 0 ) )
		{
			return ( FALSE );
		}

		IDOS->Delay( 1 );

		if ( AX88772_Write( unit, AX_RESET, 0, 0 ) )
		{
			return ( FALSE );
		}

		IDOS->Delay( 3 );

		if ( AX88772_Write( unit, AX_RESET, AX_RESET_INT_PHY | AX_RESET_EXT_PHY, 0 ) )
		{
			return ( FALSE );
		}
	}
	else if ( AX88772_Write( unit, AX_RESET, AX_POWER_DOWN_PHY | AX_RESET_EXT_PHY, 0 ) )
	{
		return ( FALSE );
	}

	IDOS->Delay( 8 );

	return ( TRUE );
}

// --

static S32
ResetLaterPHY( struct devUnit *unit, U8 chip )
{
	U8 select;

	select = AX_PHY_SERIAL | ( unit->unit_AXPHY == AX_PHY_INTERNAL );

	if ( AX88772_Write( unit, AX_SELECT_PHY, select, 0 ) )
	{
		return ( FALSE );
	}

	IDOS->Delay( 1 );

	if ( AX88772_Write( unit, AX_RESET, AX_POWER_DOWN_PHY | AX_RESET_INT_PHY, 0 ) )
	{
		return ( FALSE );
	}

	IDOS->Delay( 1 );

	if ( AX88772_Write( unit, AX_RESET, AX_RESET_INT_PHY, 0 ) )
	{
		return ( FALSE );
	}

	IDOS->Delay( 8 );

	if ( ( AX88772_Write( unit, AX_RESET, 0, 0 ) ) ||
		( AX88772_Write( unit, AX_RESET, AX_RESET_INT_PHY, 0 ) ) )
	{
		return ( FALSE );
	}

	IDOS->Delay( 10 );

	if ( chip == AX_CHIP_772B )
	{
		// Select the 2 KiB receive queue configuration of the B revision.
		return ( ! AX88772_Write( unit, AX_BULK_QUEUE, 0x8000, 0x8001 ) );
	}

	return ( TRUE );
}

// --

static S32
RestorePHYDefaults( struct devUnit *unit )
{
	static const U16 defaults[] = { 0x442c, 0x03c8, 0x4044 };

	U16 index;
	U16 value;

	for( index = 0; index < sizeof( defaults ) / sizeof( defaults[0] ); index++ )
	{
		if ( ! AX88772_ReadPHY( unit, 0x14 + index, & value ) )
		{
			return ( FALSE );
		}

		if ( ( value != defaults[index] ) && ( ! AX88772_WritePHY( unit, 0x14 + index, defaults[index] ) ) )
		{
			return ( FALSE );
		}
	}

	return ( TRUE );
}

// --

static S32
StartNegotiation( struct devUnit *unit, U8 chip )
{
	U16 id1;
	U16 id2;
	U16 advertisement;
	S32 configured;

	if ( ! AX88772_StartMDIO( unit ) )
	{
		return ( FALSE );
	}

	advertisement = MII_ADV_SELECTOR | MII_ADV_10_HALF | MII_ADV_10_FULL |
		MII_ADV_100_HALF | MII_ADV_100_FULL | MII_ADV_PAUSE;

	configured = ( ( chip != AX_CHIP_772A ) || ( unit->unit_AXPHY != AX_PHY_INTERNAL ) ||
		( RestorePHYDefaults( unit ) ) ) &&
		( AX88772_ReadPHY( unit, MII_PHY_ID1, & id1 ) ) &&
		( AX88772_ReadPHY( unit, MII_PHY_ID2, & id2 ) ) &&
		( ( id1 ) || ( id2 ) ) && ( ( id1 != 0xffff ) || ( id2 != 0xffff ) ) &&
		( AX88772_WritePHY( unit, MII_ADVERTISE, advertisement ) ) &&
		( AX88772_WritePHY( unit, MII_CONTROL, MII_AUTONEG | MII_RESTART ) );

	return ( ( AX88772_EndMDIO( unit ) ) && ( configured ) );
}

// --

int
Driver_Setup( struct devUnit *unit )
{
	U8 phy[2];
	U8 chip;
	U16 gpio;
	S32 reset;

	if ( AX88772_Read( unit, AX_MII_STATUS, 0, 0, & chip, sizeof( chip ) ) )
	{
		return ( FALSE );
	}

	chip &= AX_CHIP_MASK;

	if ( ( chip != AX_CHIP_772 ) && ( chip != AX_CHIP_772A ) && ( chip != AX_CHIP_772B ) )
	{
		IExec->DebugPrintF( "AX88772: unsupported chip code $%02lx\n", (U32) chip );

		return ( FALSE );
	}

	gpio = AX_GPIO_RELOAD;

	if ( chip == AX_CHIP_772 )
	{
		gpio |= AX_GPIO_PHY_POWER;
	}

	if ( AX88772_Write( unit, AX_WRITE_GPIO, gpio, 0 ) )
	{
		return ( FALSE );
	}

	IDOS->Delay( 1 );

	if ( ( ! ReadStation( unit, chip ) ) ||
		( AX88772_Read( unit, AX_READ_PHY, 0, 0, phy, sizeof( phy ) ) ) ||
		( phy[1] >= 32 ) || ( ! unit->unit_USBBulkOut->EndPointMax ) )
	{
		return ( FALSE );
	}

	unit->unit_AXPHY = phy[1];
	unit->unit_AXMedium = AX_MEDIUM_RX | AX_MEDIUM_SPEED100 | AX_MEDIUM_DUPLEX | AX_MEDIUM_AC;

	reset = ( chip == AX_CHIP_772 ) ? ResetPHY( unit ) : ResetLaterPHY( unit, chip );

	if ( ( ! reset ) ||
		( AX88772_Write( unit, AX_RX_CONTROL, 0, 0 ) ) ||
		( AX88772_Write( unit, AX_WRITE_IPG, 0x0c15, 0x12 ) ) ||
		( ! Wired_SetMacAddress( unit, unit->unit_CurrentAddress ) ) ||
		( AX88772_Write( unit, AX_WRITE_MEDIUM, unit->unit_AXMedium, 0 ) ) ||
		( AX88772_Write( unit, AX_RX_CONTROL, AX_RX_ON | AX_RX_BROADCAST | AX_RX_ALL_MULTI, 0 ) ) ||
		( ! StartNegotiation( unit, chip ) ) )
	{
		return ( FALSE );
	}

	unit->unit_ControlNeedsSetup = FALSE;

	IExec->DebugPrintF( "AX88772: chip $%02lx, PHY %lu, station %02lx:%02lx:%02lx:%02lx:%02lx:%02lx\n",
		(U32) chip, (U32) unit->unit_AXPHY, (U32) unit->unit_DefaultAddress[0], (U32) unit->unit_DefaultAddress[1],
		(U32) unit->unit_DefaultAddress[2], (U32) unit->unit_DefaultAddress[3],
		(U32) unit->unit_DefaultAddress[4], (U32) unit->unit_DefaultAddress[5] );

	return ( TRUE );
}

// --
