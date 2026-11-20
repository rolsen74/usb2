
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Manager.h"

// --

PTR
_manager_Close( struct DeviceManagerInterface *Self, struct IORequest *ioreq )
{
	struct EthernetBase *ethBase;
	struct EthernetUnit *unit;
	struct IOSana2Req *sana;
	struct BufferManagement *opener;

	USBDEBUG( "RT2870 : _manager_Close : IOReq %p", ioreq );

	ethBase = (PTR) Self->Data.LibBase;

	ObtainSemaphore( & ethBase->eth_ETH_Semaphore );

	// Check we have an Unit
	unit = (PTR) ioreq->io_Unit;

	if ( ! unit )
	{
		USBERROR( "RT2870 : Unit NULL Pointer" );
		goto bailout;
	}

	if ( ioreq->io_Message.mn_Length >= sizeof( *sana ) )
	{
		sana = (PTR) ioreq;
		opener = sana->ios2_BufferManagement;

		if ( opener )
		{
			// A dequeued command may still be using this opener's callback.
			ObtainSemaphore( & unit->unit_WorkLock );

			ObtainSemaphore( & unit->unit_CommandLock );

			AbortPendingCommands( unit, NULL, opener );

			ReleaseSemaphore( & unit->unit_CommandLock );

			FreeVec( opener );

			sana->ios2_BufferManagement = NULL;

			ReleaseSemaphore( & unit->unit_WorkLock );
		}
	}

	ioreq->io_Device = NULL;
	ioreq->io_Unit = NULL;

	if ( --unit->unit_Unit.unit_OpenCnt == 0 )
	{
		ethBase->eth_ETH_Units[ unit->unit_UnitNr ] = NULL;

		Unit_Free( unit );
	}

	// Update Device open count
	if ( ethBase->eth_Base.dd_Library.lib_OpenCnt )
	{
		ethBase->eth_Base.dd_Library.lib_OpenCnt--;
	}

	// Check if we can expunge our self
	if ( ! ethBase->eth_Base.dd_Library.lib_OpenCnt )
	{
		if ( ethBase->eth_Base.dd_Library.lib_Flags & LIBF_DELEXP )
		{
			ReleaseSemaphore( & ethBase->eth_ETH_Semaphore );

			return ( _manager_Expunge( Self ) );
		}
	}

bailout:

	ReleaseSemaphore( & ethBase->eth_ETH_Semaphore );

	return ( NULL );
}

// --
