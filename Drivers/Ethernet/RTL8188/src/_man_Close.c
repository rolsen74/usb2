
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#include "All.h"

// --

static PTR _manager_Close( struct DeviceManagerInterface *Self, struct IORequest *ioreq )
{
struct EthernetBase *ethBase;
struct EthernetUnit *unit;
PTR seglist;

	USBDEBUG( "RTL8188 : _manager_Close : IOReq %p", ioreq );

	seglist = NULL;

	ethBase = (PTR) Self->Data.LibBase;

	ObtainSemaphore( & ethBase->eth_ETH_Semaphore );

	// Check we have an Unit
	unit = (PTR) ioreq->io_Unit;

	if ( ! unit )
	{
		USBERROR( "RTL8188 : Unit NULL Pointer" );
		goto bailout;
	}

	// Check Unit open count
	if ( unit->unit_Unit.unit_OpenCnt )
	{
		unit->unit_Unit.unit_OpenCnt--;
	}

	if ( unit->unit_Unit.unit_OpenCnt )
	{
		// Hmm since we only have one Opener pr unit.. this is an error
		USBERROR( "RTL8188 : Unit open count not Zero : OpenCnt %ld", unit->unit_Unit.unit_OpenCnt );
		goto bailout;
	}

	// Clear Open Array Ptr
	ethBase->eth_ETH_Units[ unit->unit_UnitNr ] = NULL;

	// Free Unit
	Unit_Free( unit );
	unit = NULL;

	// Make sure unit can't be closed again
	ioreq->io_Device = NULL;
	ioreq->io_Unit = NULL;

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
			seglist = _manager_Expunge( Self );
		}
	}

bailout:

	ReleaseSemaphore( & ethBase->eth_ETH_Semaphore );

	return( seglist );
}

// --
