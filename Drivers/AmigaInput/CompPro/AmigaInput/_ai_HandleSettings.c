
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "CompPro.h"

// --

S32 _AI_HandleSettings( struct AINSubIFace *Self UNUSED, AIN_DeviceID ID UNUSED, AIN_DriverSettings *settings UNUSED, BOOL load UNUSED, BOOL use UNUSED )
{
	DebugPrintF( "_AI_HandleSettings\n" );
	return( 0 );
}

// --
