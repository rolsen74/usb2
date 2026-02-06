
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "CompPro.h"

// --

S32 _main_RunInterface( struct USBFDIFace *Self UNUSED, struct USBFDStartupMsg *msg UNUSED )
{
	return( USBERR_UNSUPPORTED );
}

// --
