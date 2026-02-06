
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "CompPro.h"

// --

S32 _AI_Read_KBD( struct AINSubIFace *Self UNUSED, AIN_DeviceID ID UNUSED, STR puffer UNUSED )
{
	DebugPrintF( "_AI_Read_KBD\n" );
	return( 0 );
}

// --
