
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

S32
Cmd_BroadCast( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	memset( ioreq->ios2_DstAddr, 0xff, 6 );

	return ( Cmd_Write( unit, ioreq ) );
}

// --
