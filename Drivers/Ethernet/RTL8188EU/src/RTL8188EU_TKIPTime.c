
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

U64
RTL8188EU_TKIPTime( void )
{
	struct TimeVal time;

	IRTL8188Timer->GetUpTime( & time );

	return( (U64) time.Seconds * 1000000 + time.Microseconds );
}

// --
