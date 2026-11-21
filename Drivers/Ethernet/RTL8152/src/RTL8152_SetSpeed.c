
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Controller.h"

// --

int
RTL8152_SetSpeed( struct devUnit *unit, int AutoNeg, uint16 Speed, int Duplex )
{
U32 cnt;
uint16 anar;
uint16 bmcr;
uint16 gbcr;


	// --

	if ( myReadMDIO( unit, MII_ADVERTISE, & anar ) )
	{
		return ( FALSE );
	}

    anar &= ~( ADVERTISE_10HALF | ADVERTISE_10FULL | ADVERTISE_100HALF | ADVERTISE_100FULL );

	if ( unit->unit_Support_GMII )
    {
		if ( myReadMDIO( unit, MII_CTRL1000, & gbcr ) )
		{
			return ( FALSE );
		}

	    gbcr &= ~( ADVERTISE_1000HALF | ADVERTISE_1000FULL );
    }
    else
    {
	    gbcr = 0;
    }

    if ( AutoNeg == FALSE )
    {
	    if ( Speed == SPEED_10 )
	    {
		    bmcr  = 0;
		    anar |= ADVERTISE_10HALF | ADVERTISE_10FULL;
	    }
		else if (( Speed == SPEED_1000 ) && ( unit->unit_Support_GMII ))
	    {
		    bmcr  = BMCR_SPEED1000;
		    gbcr |= ADVERTISE_1000HALF | ADVERTISE_1000FULL;
	    }
	    else // SPEED_100
	    {
		    bmcr  = BMCR_SPEED100;
		    anar |= ADVERTISE_100HALF | ADVERTISE_100FULL;
	    }

	    if ( Duplex )
	    {
		    bmcr |= BMCR_FULLDPLX;
	    }
    }
    else
    {
	    if ( Speed == SPEED_10 )
	    {
		    if ( Duplex )
		    {
			    anar |= ADVERTISE_10HALF | ADVERTISE_10FULL;
		    }
		    else
		    {
			    anar |= ADVERTISE_10HALF ;
		    }
	    }
		else if (( Speed == SPEED_1000 ) && ( unit->unit_Support_GMII ))
	    {
		    if ( Duplex )
		    {
			    anar |= ADVERTISE_10HALF | ADVERTISE_10FULL | ADVERTISE_100HALF | ADVERTISE_100FULL;
			    gbcr |= ADVERTISE_1000HALF | ADVERTISE_1000FULL;
		    }
		    else
		    {
			    anar |= ADVERTISE_10HALF | ADVERTISE_100HALF;
			    gbcr |= ADVERTISE_1000HALF;
		    }
	    }
	    else // SPEED_100
	    {
		    if ( Duplex )
		    {
			    anar |= ADVERTISE_10HALF | ADVERTISE_10FULL | ADVERTISE_100HALF | ADVERTISE_100FULL;
		    }
		    else
		    {
			    anar |= ADVERTISE_10HALF | ADVERTISE_100HALF;
		    }
	    }

	    bmcr = BMCR_ANENABLE | BMCR_ANRESTART;
    }

	if ( ( unit->unit_Support_GMII ) && ( myWriteMDIO( unit, MII_CTRL1000, gbcr ) ) )
	{
		return ( FALSE );
	}

	if ( myWriteMDIO( unit, MII_ADVERTISE, anar ) )
	{
		return ( FALSE );
	}

	if ( myWriteMDIO( unit, MII_BMCR, bmcr | BMCR_RESET ) )
	{
		return ( FALSE );
	}

	for( cnt = 0; cnt < 50; cnt++ )
	{
		IDOS->Delay( 1 );

		if ( myReadMDIO( unit, MII_BMCR, & bmcr ) )
		{
			return ( FALSE );
		}

		if ( ! ( bmcr & BMCR_RESET ) )
		{
			return ( TRUE );
		}
	}

	IExec->DebugPrintF( "RTL8152: PHY reset timed out\n" );

	return ( FALSE );
}

// --
