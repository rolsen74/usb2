
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
RT2870_ClearKeys( struct EthernetUnit *unit )
{
	volatile U8 *keys;
	U32 i;

	keys = (volatile U8 *) unit->unit_Keys;

	for( i = 0; i < sizeof( unit->unit_Keys ); i++ )
	{
		keys[i] = 0;
	}

	keys = (volatile U8 *) unit->unit_IGTK;

	for( i = 0; i < sizeof( unit->unit_IGTK ); i++ )
	{
		keys[i] = 0;
	}

	unit->unit_ManagementRxPN = 0;
	unit->unit_ActiveKeys = 0;
	unit->unit_TxNumber = 0;
}

// --
