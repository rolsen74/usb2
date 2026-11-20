
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
RT2870_DropLink( struct EthernetUnit *unit, S32 detached )
{
	ObtainSemaphore( & unit->unit_CommandLock );

	unit->unit_Channel = 0;

	ReleaseSemaphore( & unit->unit_CommandLock );

	RT2870_StopBeacon( unit, detached );
}

// --
