
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

S32
_manager_Open(
	struct DeviceManagerInterface *Self,
	struct IORequest *ioreq,
	U32 unitnum,
	U32 flags )
{
	struct EthernetBase *ethBase;
	struct EthernetUnit *unit;
	struct IOSana2Req *sana;
	struct BufferManagement *opener;

	S32 error;
	S32 detached;

	USBDEBUG( "SiS163U : _manager_Open" );

	error = IOERR_OPENFAIL;
	opener = NULL;
	sana = NULL;

	ethBase = (PTR) Self->Data.LibBase;

	ObtainSemaphore( & ethBase->eth_ETH_Semaphore );

	/* Add Expunge protection */
	ethBase->eth_Base.dd_Library.lib_OpenCnt++;

	/* Check request size */
	if ( ioreq->io_Message.mn_Length < sizeof( struct IORequest ))
	{
		USBERROR( "SiS163U : Invalid request size : Size %ld", (S32) ioreq->io_Message.mn_Length );
		error = IOERR_OPENFAIL;
		goto bailout;
	}

	/* Check for valid Unit number (must be 0-31) */
	if ( unitnum >= MAX_ETH_UNITS )
	{
		USBERROR( "SiS163U : Invalid unit number : Unit nr #%lu", unitnum );
		error = IOERR_OPENFAIL;
		goto bailout;
	}

	if ( flags & SANA2OPF_PROM )
	{
		goto bailout;
	}

	if ( ioreq->io_Message.mn_Length >= sizeof( *sana ) )
	{
		sana = (PTR) ioreq;

		opener = AllocVecTags( sizeof( *opener ), AVT_ClearWithValue, 0, TAG_END );

		if ( ! opener )
		{
			goto bailout;
		}

		if ( sana->ios2_BufferManagement )
		{
			opener->bm_CopyToBuff = (PTR) GetTagData( S2_CopyToBuff, 0, sana->ios2_BufferManagement );

			opener->bm_CopyFromBuff = (PTR) GetTagData( S2_CopyFromBuff, 0, sana->ios2_BufferManagement );
		}
	}

	unit = ethBase->eth_ETH_Units[ unitnum ];

	if ( unit )
	{
		ObtainSemaphore( & unit->unit_CommandLock );

		detached = unit->unit_Detached;

		ReleaseSemaphore( & unit->unit_CommandLock );

		if ( ( unit->unit_Exclusive ) || ( detached ) || ( flags & SANA2OPF_MINE ) )
		{
			goto bailout;
		}

		goto opened;
	}

	/* Create new unit */
	unit = Unit_Alloc( unitnum );

	if ( ! unit )
	{
		USBERROR( "SiS163U : Error creating Unit" );
		error = IOERR_OPENFAIL;
		goto bailout;
	}

	if ( ! Unit_Startup( unit ) )
	{
		Unit_Free( unit );

		error = IOERR_OPENFAIL;

		goto bailout;
	}

	unit->unit_Exclusive = ( flags & SANA2OPF_MINE ) != 0;

	ethBase->eth_ETH_Units[unitnum] = unit;

opened:

	ioreq->io_Unit = (PTR) unit;
	unit->unit_Unit.unit_OpenCnt++;

	if ( sana )
	{
		sana->ios2_BufferManagement = opener;
	}

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
		FreeVec( opener );

		ioreq->io_Device = NULL;
		ioreq->io_Unit = NULL;
	}

	ioreq->io_Error = error;

	/* Remove Expunge protection */
	ethBase->eth_Base.dd_Library.lib_OpenCnt--;

	ReleaseSemaphore( & ethBase->eth_ETH_Semaphore );

	return ( error );
}

// --
