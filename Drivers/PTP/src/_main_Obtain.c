
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

U32 _main_Obtain( struct USB2_DriverIFace *Self )
{
	MYDEBUG( "PTP : _main_Obtain" );

	Self->Data.RefCount++;

	return( Self->Data.RefCount );
}

// --
