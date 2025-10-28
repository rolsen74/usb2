
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
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
