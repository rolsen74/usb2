
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "CompPro.h"

// --

U32 _AI_SetEventMode( struct AINSubIFace *Self UNUSED, AIN_DeviceID ID UNUSED, U32 MOD UNUSED )
{
	DebugPrintF( "_AI_SetEventMode\n" );
	return( 0 );
}

// --
