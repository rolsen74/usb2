
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
ReadMedium( struct devUnit *unit, U16 *medium )
{
	U16 status;
	U16 control;
	U16 advertised;
	U16 partner;
	U16 common;

	// Link status latches low until it is read once.
	if ( ( ! AX88772_ReadPHY( unit, MII_STATUS, & status ) ) ||
		( ! AX88772_ReadPHY( unit, MII_STATUS, & status ) ) )
	{
		return ( FALSE );
	}

	*medium = 0;

	if ( ! ( status & MII_LINK_UP ) )
	{
		return ( TRUE );
	}

	if ( ! AX88772_ReadPHY( unit, MII_CONTROL, & control ) )
	{
		return ( FALSE );
	}

	if ( ( control & MII_AUTONEG ) && ( ! ( status & MII_NEGOTIATED ) ) )
	{
		return ( TRUE );
	}

	*medium = AX_MEDIUM_AC | AX_MEDIUM_RX;

	if ( ! ( control & MII_AUTONEG ) )
	{
		*medium |= ( control & MII_SPEED100 ) ? AX_MEDIUM_SPEED100 : 0;
		*medium |= ( control & MII_DUPLEX ) ? AX_MEDIUM_DUPLEX : 0;

		return ( TRUE );
	}

	if ( ( ! AX88772_ReadPHY( unit, MII_ADVERTISE, & advertised ) ) ||
		( ! AX88772_ReadPHY( unit, MII_PARTNER, & partner ) ) )
	{
		return ( FALSE );
	}

	common = advertised & partner;

	if ( common & ( MII_ADV_100_FULL | MII_ADV_100_HALF ) )
	{
		*medium |= AX_MEDIUM_SPEED100;
		*medium |= ( common & MII_ADV_100_FULL ) ? AX_MEDIUM_DUPLEX : 0;
	}
	else if ( common & MII_ADV_10_FULL )
	{
		*medium |= AX_MEDIUM_DUPLEX;
	}

	if ( ( *medium & AX_MEDIUM_DUPLEX ) && ( common & MII_ADV_PAUSE ) )
	{
		*medium |= AX_MEDIUM_RX_PAUSE | AX_MEDIUM_TX_PAUSE;
	}

	return ( TRUE );
}

// --

S32
Wired_PHYLink( struct devUnit *unit )
{
	U16 medium;
	S32 valid;

	if ( ! AX88772_StartMDIO( unit ) )
	{
		return ( -1 );
	}

	valid = ReadMedium( unit, & medium );

	if ( ( ! AX88772_EndMDIO( unit ) ) || ( ! valid ) )
	{
		return ( -1 );
	}

	if ( medium != unit->unit_AXMedium )
	{
		if ( AX88772_Write( unit, AX_WRITE_MEDIUM, medium, 0 ) )
		{
			return ( -1 );
		}

		unit->unit_AXMedium = medium;

		if ( medium )
		{
			IExec->DebugPrintF( "AX88772: link %lu Mbps %s duplex\n",
				( medium & AX_MEDIUM_SPEED100 ) ? 100UL : 10UL,
				( medium & AX_MEDIUM_DUPLEX ) ? "full" : "half" );
		}
	}

	return ( medium != 0 );
}

// --
