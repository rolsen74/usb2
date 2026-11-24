
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

static void
__myProcess_Drain_MsgPort( struct PrinterUnit *unit )
{
	struct IORequest *request;

	if ( unit->unit_PRT_Begin_MsgPort )
	{
		while ( ( request = (PTR)GetMsg( unit->unit_PRT_Begin_MsgPort ) ) )
		{
			request->io_Error = ParErr_NotOpen;

			Printer_Reply( request );
		}
	}
}

// --

static void
__myProcess_Free_MsgPorts( struct PrinterUnit *unit )
{
	__myProcess_Drain_MsgPort( unit );

	if ( unit->unit_PRT_Abort_MsgPort )
	{
		FreeSysObject( ASOT_PORT, unit->unit_PRT_Abort_MsgPort );
	}

	if ( unit->unit_PRT_Begin_MsgPort )
	{
		FreeSysObject( ASOT_PORT, unit->unit_PRT_Begin_MsgPort );
	}
}

// --
