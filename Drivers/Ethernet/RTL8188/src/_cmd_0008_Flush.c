
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Firmware.h"
#include "Commands.h"
#include "Radio.h"

// --

void
Cmd_Flush( struct EthernetUnit *unit, struct IOSana2Req *ioreq UNUSED )
{
	RTL8188EU_AbortData( unit, IOERR_ABORTED, S2WERR_GENERIC_ERROR );
}

// --
