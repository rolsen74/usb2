
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
RT2870_TKIPTime( void )
{
	struct TimeVal time;

	IRT2870Timer->GetUpTime( & time );

	return ( (U64) time.Seconds * 1000000 + time.Microseconds );
}

// --
