
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define RT2870_WCID_ENTRY	( 0x1800 + RT2870_AP_WCID * 8 )
#define RT2870_WCID_ATTR	( 0x6800 + RT2870_AP_WCID * 4 )

// --

S32
RT2870_SetStationPeer( struct EthernetUnit *unit, S32 enable )
{
	struct USB2_EPResource *control;

	const U8 *peer;
	U32 low;
	U32 high;
	S32 error;

	control = unit->unit_USB_Register->Res_Control;
	peer = unit->unit_Peer;
	low = 0;
	high = 0;

	if ( enable )
	{
		low = peer[0] | ( (U32) peer[1] << 8 ) | ( (U32) peer[2] << 16 ) | ( (U32) peer[3] << 24 );
		high = peer[4] | ( (U32) peer[5] << 8 );
	}

	// The WCID identifies the AP while encryption remains in software.
	error = RT2870_WriteRegister( control, RT2870_WCID_ATTR, 0 );

	if ( ! error )
	{
		error = RT2870_WriteRegister( control, RT2870_WCID_ENTRY, low );
	}

	if ( ! error )
	{
		error = RT2870_WriteRegister( control, RT2870_WCID_ENTRY + 4, high );
	}

	if ( ! error )
	{
		unit->unit_PeerConfigured = enable;
	}

	return ( error );
}

// --
