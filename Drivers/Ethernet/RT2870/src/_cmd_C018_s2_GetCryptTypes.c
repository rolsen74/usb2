
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Commands.h"

// --

void
Cmd_GetCryptTypes( struct EthernetUnit *unit UNUSED, struct IOSana2Req *ioreq )
{
	static const U8 types[] = { S2ENC_CCMP, S2ENC_TKIP };

	Sana2_GetCryptTypes( ioreq, types, IRT2870Timer ? sizeof( types ) : sizeof( types ) - 1 );
}

// --
