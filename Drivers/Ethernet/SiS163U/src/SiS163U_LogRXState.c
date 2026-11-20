
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

void
SiS163U_LogRXState( struct EthernetUnit *unit, const char *stage )
{
	struct USB2_EPResource *control;

	U32 reg70;
	U32 reg9c;
	U32 counter;
	S32 error;

	control = unit->unit_USB_Register->Res_Control;

	if ( ( error = SiS163U_ReadRegister( control, 0x0070, & reg70 ) ) ||
		( error = SiS163U_ReadRegister( control, 0x009c, & reg9c ) ) ||
		( error = SiS163U_ReadRegister( control, 0x0090, & counter ) ) )
	{
		USBERROR( "SiS163U: RX state %s unavailable, USB error %ld", stage, error );

		return;
	}

	USBINFO( "SiS163U: RX state %s, $0070 = $%08lx, $009C = $%08lx, $0090 = $%08lx",
		stage, reg70, reg9c, counter );
}

// --
