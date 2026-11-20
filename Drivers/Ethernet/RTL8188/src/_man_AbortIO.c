
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Manager.h"
#include "Radio.h"
#include "WiFiCommand.h"

// --

void
_manager_AbortIO( struct DeviceManagerInterface *Self UNUSED, struct IORequest *ioreq )
{
	struct EthernetUnit *unit;

	if ( ( ioreq->io_Command != USB2WIRELESS_CMD_PROMPT ) && ( ioreq->io_Command != S2_ONEVENT ) && ( ioreq->io_Command != S2_SAMPLE_THROUGHPUT ) &&
		( ioreq->io_Command != S2_SETKEY ) && ( ioreq->io_Command != S2_READMGMT ) && ( ioreq->io_Command != S2_SETOPTIONS ) &&
		( ioreq->io_Command != S2_WRITEMGMT ) && ( ioreq->io_Command != CMD_READ ) &&
		( ioreq->io_Command != S2_READORPHAN ) && ( ioreq->io_Command != CMD_WRITE ) &&
		( ioreq->io_Command != S2_MULTICAST ) && ( ioreq->io_Command != S2_BROADCAST ) )
	{
		return;
	}

	unit = (PTR) ioreq->io_Unit;

	if ( unit )
	{
		ObtainSemaphore( & unit->unit_CommandLock );

		AbortPendingCommands( unit, ioreq, NULL );

		ReleaseSemaphore( & unit->unit_CommandLock );
	}
}

// --
