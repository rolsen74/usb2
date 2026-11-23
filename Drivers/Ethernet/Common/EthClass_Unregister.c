
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef __USE_INLINE__
#define __USE_INLINE__
#endif

#include "All.h"
#include "EthClass.h"

// --

void
EthClass_Unregister( struct EthClassPort *port )
{
	if ( port->Unit )
	{
		port->Interface->Unregister( port->Unit );

		port->Unit = NULL;
	}

	if ( port->Interface )
	{
		DropInterface( (PTR) port->Interface );

		port->Interface = NULL;
	}

	if ( port->Request.io_Device )
	{
		CloseDevice( & port->Request );

		port->Request.io_Device = NULL;
	}
}

// --
