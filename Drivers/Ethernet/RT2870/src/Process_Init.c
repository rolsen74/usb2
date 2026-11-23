
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

S32
Process_Init( struct EthernetUnit *unit, struct USB2_DriverMessage *msg )
{
	struct USB2_Register *reg;

	reg = USB2_Reg_RegisterTags( USB2Tag_Reg_DriverMessage, msg,
		USB2Tag_Reg_Title, "RT2870", TAG_END );

	if ( ! reg )
	{
		USBERROR( "RT2870: unit %lu USB registration failed", unit->unit_UnitNr );

		return ( FALSE );
	}

	unit->unit_USB_Register = reg;

	return ( RT2870_Probe( unit ) );
}

// --
