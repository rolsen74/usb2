
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

void
RTL8152_SetSpeed( struct devUnit *unit, int AutoNeg, uint16 Speed, int Duplex )
{
U32 cnt;
uint16 anar;
uint16 bmcr;
uint16 gbcr;


	// --

	myReadMDIO( unit, MII_ADVERTISE, & anar );

    anar &= ~( ADVERTISE_10HALF | ADVERTISE_10FULL | ADVERTISE_100HALF | ADVERTISE_100FULL );

	if ( unit->unit_Support_GMII )
    {
		myReadMDIO( unit, MII_CTRL1000, & gbcr );

	    gbcr &= ~( ADVERTISE_10HALF | ADVERTISE_10FULL );
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

	// Do Always reset PHY?
    bmcr |= BMCR_RESET;


	if ( unit->unit_Support_GMII )
    {
	    myWriteMDIO( unit, MII_CTRL1000, gbcr );
    }

    myWriteMDIO( unit, MII_ADVERTISE, anar );
    myWriteMDIO( unit, MII_BMCR, bmcr );

    if ( bmcr & BMCR_RESET )
    {
	    for( cnt=0 ; cnt<50 ; cnt++ )
	    {
			myReadMDIO( unit, MII_BMCR, & bmcr );

			if ( bmcr & BMCR_RESET )
		    {
			    break;
		    }
		    else
		    {
			    IDOS->Delay( 1 );
		    }
	    }
    }

}

// --
