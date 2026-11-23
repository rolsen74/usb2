
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
	static const U8 types[] = { S2ENC_NONE };

	Sana2_GetCryptTypes( ioreq, types, sizeof( types ) );
}

// --
