
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

APTR
Device_Close( struct DeviceManagerInterface *Self, struct IOSana2Req *ioreq )
{
struct devBase *devBase;
struct devUnit *unit;

	devBase = (PTR) Self->Data.LibBase;
	unit = (PTR) ioreq->ios2_Req.io_Unit;

	IExec->ObtainSemaphore( & devBase->dev_GlobalLock );

	ioreq->ios2_Req.io_Unit = NULL;
	ioreq->ios2_Req.io_Device = NULL;

	if ( --unit->unit_Base.unit_OpenCnt == 0 )
	{
		devBase->dev_Units[ unit->unit_UnitNumber ] = NULL;

		unit_Stop( unit );

		IExec->FreeVec( unit );
	}

	if ( ioreq->ios2_BufferManagement )
	{
		IExec->FreeVec( ioreq->ios2_BufferManagement );

		ioreq->ios2_BufferManagement = NULL;
	}

	devBase->dev_Base.dd_Library.lib_OpenCnt--;

	if ( ( ! devBase->dev_Base.dd_Library.lib_OpenCnt ) &&
		( devBase->dev_Base.dd_Library.lib_Flags & LIBF_DELEXP ) )
	{
		IExec->ReleaseSemaphore( & devBase->dev_GlobalLock );

		return ( Device_Expunge( Self ) );
	}

	IExec->ReleaseSemaphore( & devBase->dev_GlobalLock );

	return ( NULL );
}

// --
