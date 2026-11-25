
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

#define PEG_MAC_STOP_RETRIES 3
#define PEG_MAC_ENABLE_BITS ( PEG_RX_ENABLE | PEG_TX_ENABLE )

// --

static S32
ReadMedium( struct devUnit *unit, U8 *mode )
{
	U16 status;
	U16 control;
	U16 advertised;
	U16 partner;
	U16 common;
	U8 phy;

	phy = unit->unit_PegasusPHY;

	// The first status read clears the latched link-down bit.
	if ( ( ! Pegasus_ReadPHY( unit, phy, PEG_MII_STATUS, & status ) ) ||
		( ! Pegasus_ReadPHY( unit, phy, PEG_MII_STATUS, & status ) ) ||
		( status == 0xffff ) )
	{
		return ( -1 );
	}

	if ( ! ( status & PEG_MII_LINK ) )
	{
		return ( FALSE );
	}

	if ( ! Pegasus_ReadPHY( unit, phy, PEG_MII_CONTROL, & control ) )
	{
		return ( -1 );
	}

	*mode = 0;

	if ( ! ( control & PEG_MII_AUTONEG ) )
	{
		*mode = ( control & PEG_MII_SPEED100 ) ? PEG_MAC_SPEED100 : 0;
		*mode |= ( control & PEG_MII_FULL_DUPLEX ) ? PEG_MAC_FULL_DUPLEX : 0;

		return ( TRUE );
	}

	if ( ! ( status & PEG_MII_NEGOTIATED ) )
	{
		return ( FALSE );
	}

	if ( ( ! Pegasus_ReadPHY( unit, phy, PEG_MII_ADVERTISE, & advertised ) ) ||
		( ! Pegasus_ReadPHY( unit, phy, PEG_MII_PARTNER, & partner ) ) )
	{
		return ( -1 );
	}

	common = advertised & partner;

	if ( common & PEG_MII_100_FULL )
	{
		*mode = PEG_MAC_SPEED100 | PEG_MAC_FULL_DUPLEX;
	}
	else if ( common & PEG_MII_100_HALF )
	{
		*mode = PEG_MAC_SPEED100;
	}
	else if ( common & PEG_MII_10_FULL )
	{
		*mode = PEG_MAC_FULL_DUPLEX;
	}
	else if ( ! ( common & PEG_MII_10_HALF ) )
	{
		// Carrier alone does not resolve the speed of a forced link partner.
		return ( FALSE );
	}

	return ( TRUE );
}

// --

S32
Pegasus_StopTraffic( struct devUnit *unit )
{
	U8 actual;

	if ( unit->unit_PegasusMACState == PEG_MAC_STOPPED )
	{
		return ( TRUE );
	}

	unit->unit_PegasusMACState = PEG_MAC_UNVERIFIED;
	unit->unit_PegasusStatusPending = FALSE;

	if ( ( Pegasus_WriteByte( unit, PEG_ETH_CTRL0, 0 ) ) &&
		( Pegasus_Read( unit, PEG_ETH_CTRL0, & actual, 1 ) ) &&
		( ! ( actual & PEG_MAC_ENABLE_BITS ) ) )
	{
		unit->unit_PegasusMACState = PEG_MAC_STOPPED;
		unit->unit_PegasusStopFailures = 0;

		return ( TRUE );
	}

	if ( unit->unit_PegasusStopFailures < PEG_MAC_STOP_RETRIES )
	{
		unit->unit_PegasusStopFailures++;

		IExec->DebugPrintF( "Pegasus: could not verify MAC stopped, attempt %lu\n",
			(U32) unit->unit_PegasusStopFailures );
	}

	return ( FALSE );
}

// --

static S32
StartTraffic( struct devUnit *unit, U8 mode )
{
	U8 control[3];
	U8 actual[3];

	unit->unit_PegasusMACState = PEG_MAC_UNVERIFIED;
	unit->unit_PegasusStopFailures = 0;
	unit->unit_PegasusStatusPending = FALSE;

	// The common SANA-II path filters subscribed multicast addresses.
	control[0] = PEG_MAC_ENABLE_BITS | PEG_RX_APPEND_STATUS | PEG_RX_ALL_MULTICAST;
	control[1] = mode;
	control[2] = PEG_EP3_CLEAR_STATUS;

	if ( ( ! Pegasus_Write( unit, PEG_ETH_CTRL0, control, sizeof( control ) ) ) ||
		( ! Pegasus_Read( unit, PEG_ETH_CTRL0, actual, sizeof( actual ) ) ) )
	{
		if ( unit->unit_PegasusStartFailures < 3 )
		{
			unit->unit_PegasusStartFailures++;

			IExec->DebugPrintF( "Pegasus: MAC enable write/readback failed\n" );
		}

		return ( FALSE );
	}

	if ( ( ( actual[0] & control[0] ) != control[0] ) ||
		( ( actual[1] & ( PEG_MAC_SPEED100 | PEG_MAC_FULL_DUPLEX ) ) != mode ) )
	{
		if ( unit->unit_PegasusStartFailures < 3 )
		{
			unit->unit_PegasusStartFailures++;

			IExec->DebugPrintF( "Pegasus: MAC enable mismatch, wrote $%02lx/$%02lx, read $%02lx/$%02lx\n",
				(U32) control[0], (U32) control[1], (U32) actual[0], (U32) actual[1] );
		}

		return ( FALSE );
	}

	unit->unit_PegasusMode = mode;
	unit->unit_PegasusMACState = PEG_MAC_READY;
	unit->unit_PegasusStartFailures = 0;

	IExec->DebugPrintF( "Pegasus: link %lu Mbps %s duplex, MAC $%02lx/$%02lx/$%02lx, USB bulk IN/OUT max %lu/%lu\n",
		( mode & PEG_MAC_SPEED100 ) ? 100UL : 10UL, ( mode & PEG_MAC_FULL_DUPLEX ) ? "full" : "half",
		(U32) actual[0], (U32) actual[1], (U32) actual[2],
		(U32) unit->unit_USBBulkIn->EndPointMax, (U32) unit->unit_USBBulkOut->EndPointMax );

	return ( TRUE );
}

// --

S32
Wired_PHYLink( struct devUnit *unit )
{
	S32 link;
	U8 mode;

	mode = 0;

	link = unit->unit_Online ? ReadMedium( unit, & mode ) : FALSE;

	if ( ! link )
	{
		if ( unit->unit_PegasusStopFailures < PEG_MAC_STOP_RETRIES )
		{
			Pegasus_StopTraffic( unit );
		}

		return ( FALSE );
	}

	if ( link < 0 )
	{
		return ( ( unit->unit_PegasusMACState == PEG_MAC_READY ) ? -1 : FALSE );
	}

	if ( ( unit->unit_PegasusMACState != PEG_MAC_READY ) || ( mode != unit->unit_PegasusMode ) )
	{
		return ( StartTraffic( unit, mode ) );
	}

	Pegasus_TraceMACStatus( unit );

	return ( TRUE );
}

// --
