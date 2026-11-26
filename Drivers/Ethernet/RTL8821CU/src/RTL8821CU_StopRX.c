
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
RTL8821CU_StopRX( struct EthernetUnit *unit )
{
	Wireless_StopRX( unit->unit_Bulk_In, &unit->unit_RxPending );
}

// --
