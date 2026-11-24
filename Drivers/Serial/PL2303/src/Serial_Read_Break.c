/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Serial_Read_Break( struct SerialContext *ctx, struct SerialUnit *unit )
{
	unit->BreakQuery = TRUE;

	if ( unit->ReadBreaks != (U32)-1 )
	{
		unit->ReadBreaks++;
	}

	Serial_Read_Drain( ctx, unit );
}

// --
