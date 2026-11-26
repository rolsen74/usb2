
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
SiS163U_SubmitRXCheck( struct EthernetUnit *unit )
{
	Wireless_SubmitRX( unit->unit_Bulk_Rx_Resource, &unit->unit_RxActive );
}

// --
