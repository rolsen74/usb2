
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
RTL8188EU_ClearKeys( struct EthernetUnit *unit )
{
	volatile U8 *data;
	U32 i;

	data = (volatile U8 *) unit->unit_Keys;

	for( i = 0; i < sizeof( unit->unit_Keys ); i++ )
	{
		data[i] = 0;
	}

	data = (volatile U8 *) unit->unit_IGTK;

	for( i = 0; i < sizeof( unit->unit_IGTK ); i++ )
	{
		data[i] = 0;
	}
}

// --
