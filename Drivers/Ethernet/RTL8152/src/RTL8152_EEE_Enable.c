
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
RTL8152_EEE_Enable( struct devUnit *unit, int Enable )
{
uint16 cfg1, cfg2, cfg3, data;


    myRead2(  unit, URE_PLA_EEE_CR, URE_MCU_TYPE_PLA, & data );
  
    myReadReg( unit, URE_OCP_EEE_CONFIG1, & cfg1 );
    cfg1 &= ~sd_rise_time_mask;

    myReadReg( unit, URE_OCP_EEE_CONFIG2, & cfg2 );
  
    myReadReg( unit, URE_OCP_EEE_CONFIG3, & cfg3 );
    cfg3 &= ~fast_snr_mask;

    if ( Enable )
    {
	    data |= URE_EEE_RX_EN | URE_EEE_TX_EN;
	    cfg1 |= EEE_10_CAP | EEE_NWAY_EN | TX_QUIET_EN | RX_QUIET_EN | sd_rise_time(1);
	    cfg2 |= RG_DACQUIET_EN | RG_LDVQUIET_EN;
	    cfg3 |= fast_snr(42);
    }
    else
    {
	    data &= ~( URE_EEE_RX_EN | URE_EEE_TX_EN );
	    cfg1 &= ~( EEE_10_CAP | EEE_NWAY_EN | TX_QUIET_EN | RX_QUIET_EN );
	    cfg1 |= sd_rise_time(7);
	    cfg2 &= ~( RG_DACQUIET_EN | RG_LDVQUIET_EN );
	    cfg3 |= fast_snr(511);
    }

    myWrite2( unit, URE_PLA_EEE_CR, URE_MCU_TYPE_PLA, data );

    myWriteReg( unit, URE_OCP_EEE_CONFIG1, cfg1 );
    myWriteReg( unit, URE_OCP_EEE_CONFIG2, cfg2 );
    myWriteReg( unit, URE_OCP_EEE_CONFIG3, cfg3 );

}

// --
