
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define RT2870_BCN_TIME_CFG		0x1114
#define RT2870_BCN_INTERVAL_MASK	0x0000ffff
#define RT2870_TSF_TIMER_EN		0x00010000
#define RT2870_TSF_SYNC_STA		0x00020000
#define RT2870_TBTT_TIMER_EN		0x00080000
#define RT2870_BCN_TIME_MASK		0x001fffff

// --

S32
RT2870_SetStationTiming( struct EthernetUnit *unit, U16 interval )
{
	struct USB2_EPResource *control;

	U32 config;
	U32 ticks;
	S32 error;

	control = unit->unit_USB_Register->Res_Control;
	ticks = (U32) interval * 16;

	if ( ticks > RT2870_BCN_INTERVAL_MASK )
	{
		ticks = 100 * 16;
	}

	error = RT2870_ReadRegister( control, RT2870_BCN_TIME_CFG, & config );

	if ( error )
	{
		return ( error );
	}

	config &= ~RT2870_BCN_TIME_MASK;

	if ( interval )
	{
		config |= ticks | RT2870_TSF_TIMER_EN | RT2870_TSF_SYNC_STA | RT2870_TBTT_TIMER_EN;
		unit->unit_TimingActive = TRUE;
	}

	error = RT2870_WriteRegister( control, RT2870_BCN_TIME_CFG, config );

	if ( ! error )
	{
		unit->unit_TimingActive = interval != 0;

		if ( interval )
		{
			USBINFO( "RT2870: station timing synchronized to an AP beacon interval of %lu TU", ticks / 16 );
		}
	}

	return ( error );
}

// --
