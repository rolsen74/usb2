
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

static S32 _manager_Open(
	struct DeviceManagerInterface *Self,
	struct IORequest *ioreq,
	U32 unitnum,
	U32 flags UNUSED )
{
struct EthernetBase *ethBase;
struct EthernetUnit *unit;
S32 error;

	USBDEBUG( "RTL8188 : _manager_Open" );

	error = 0;

	ethBase = (PTR) Self->Data.LibBase;

	ObtainSemaphore( & ethBase->eth_ETH_Semaphore );

	/* Add Expunge protection */
	ethBase->eth_Base.dd_Library.lib_OpenCnt++;

	/* Check request size */
	if ( ioreq->io_Message.mn_Length < sizeof( struct IORequest ))
	{
		USBERROR( "RTL8188 : Invalid request size : Size %ld", (S32) ioreq->io_Message.mn_Length );
		error = IOERR_OPENFAIL;
		goto bailout;
	}

	/* Check for valid Unit number (must be 0-31) */
	if ( unitnum >= MAX_ETH_UNITS )
	{
		USBERROR( "RTL8188 : Invalid unit number : Unit nr #%lu", unitnum );
		error = IOERR_OPENFAIL;
		goto bailout;
	}

	/* Only one opener pr. unit, check if its in use */
	if ( ethBase->eth_ETH_Units[ unitnum ] )
	{
		USBERROR( "RTL8188 : Unit alreay in use : Unit nr #%lu", unitnum );
		error = IOERR_OPENFAIL;
		goto bailout;
	}

	/* Create new unit */
	unit = Unit_Alloc( unitnum );

	if ( ! unit )
	{
		USBERROR( "RTL8188 : Error creating Unit" );
		error = IOERR_OPENFAIL;
		goto bailout;
	}

	ioreq->io_Unit = (PTR) unit;
	unit->unit_Unit.unit_OpenCnt++;
 
	/* Prevent delayed expunges */
	ethBase->eth_Base.dd_Library.lib_Flags &= ~LIBF_DELEXP;
 
	/* Success */
	error = 0;

bailout:

	if ( ! error )
	{
		ethBase->eth_Base.dd_Library.lib_OpenCnt++;
	}
	else
	{
		ioreq->io_Device = NULL;
	}

	ioreq->io_Error = error;

	/* Remove Expunge protection */
	ethBase->eth_Base.dd_Library.lib_OpenCnt--;

	ReleaseSemaphore( & ethBase->eth_ETH_Semaphore );

	return( error );
}

// --
