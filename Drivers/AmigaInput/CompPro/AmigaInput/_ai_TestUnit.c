
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "CompPro.h"

// --

U32 _AI_TestUnit( struct AINSubIFace *Self UNUSED, AIN_DeviceID ID UNUSED )
{
	DebugPrintF( "_AI_TestUnit\n" );
	return( 0 );
}

// --
