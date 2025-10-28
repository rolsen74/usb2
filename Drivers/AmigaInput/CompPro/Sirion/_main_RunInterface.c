
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

S32 _main_RunInterface( struct USBFDIFace *Self UNUSED, struct USBFDStartupMsg *msg UNUSED )
{
	return( USBERR_UNSUPPORTED );
}

// --
