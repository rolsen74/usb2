
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "MSD.h"
#include <utility/hooks.h>

// --

SEC_CODE S32
MSD_Cmd_A006_NSCmd_AddStatCallBack( struct USBBase *usbbase, struct IOStdReq *ioreq )
{
	struct MSDDisk *msddisk;
	struct MSDStatCallback *callback;
	struct Hook *hook;

	S32 reply;

	msddisk = (PTR) ioreq->io_Unit;
	hook = ioreq->io_Data;
	ioreq->io_Actual = 0;

	if ( ! msddisk )
	{
		ioreq->io_Error = TDERR_BadUnitNum;

		return ( TRUE );
	}

	MSDDEBUG( "MSD: unit %lu received ADDSTATCALLBACK ($A006)\n", msddisk->msddisk_UnitNr );

	if ( ( ! hook ) || ( ! hook->h_Entry ) )
	{
		ioreq->io_Error = IOERR_BADADDRESS;

		return ( TRUE );
	}

	usbbase->usb_IExec->ObtainSemaphore( & msddisk->msddisk_StatSemaphore );

	// Keep the registration off the request's own message node.
	for( callback = Header_Head( & msddisk->msddisk_MSDUnit_StatList ); callback; callback = Node_Next( callback ) )
	{
		if ( callback->Request == ioreq )
		{
			usbbase->usb_IExec->ReleaseSemaphore( & msddisk->msddisk_StatSemaphore );

			return ( FALSE );
		}

		if ( callback->Hook == hook )
		{
			ioreq->io_Error = IOERR_UNITBUSY;
			break;
		}
	}

	if ( msddisk->msddisk_MSDDev->msddev_Detached )
	{
		ioreq->io_Error = TDERR_DiskChanged;
	}

	if ( ! ioreq->io_Error )
	{
		callback = MEM_ALLOCVEC( sizeof( struct MSDStatCallback ), TRUE );

		if ( callback )
		{
			callback->Request = ioreq;
			callback->Hook = hook;

			NODE_ADDTAIL( & msddisk->msddisk_MSDUnit_StatList, callback );
		}
		else
		{
			ioreq->io_Error = TDERR_NoMem;
		}
	}

	reply = ioreq->io_Error != 0;

	usbbase->usb_IExec->ReleaseSemaphore( & msddisk->msddisk_StatSemaphore );

	return ( reply );
}

// --
