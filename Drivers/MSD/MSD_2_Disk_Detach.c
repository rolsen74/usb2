
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "MSD.h"

// --

SEC_CODE void MSD_Disk_Detach( struct USBBase *usbbase, struct MSDDisk *msddisk )
{
struct MSDDevice *msddev;
struct MSDStatCallback *callback;
struct IOStdReq *ioreq;

	TASK_NAME_ENTER( "MSD_Disk_Detach" );

	if ( msddisk->msddisk_Detached )
	{
		TASK_NAME_LEAVE();

		return;
	}

	USBERROR( "MSD : MSD_Disk_Detach" );

	//	--

	msddev = msddisk->msddisk_MSDDev;
	msddisk->msddisk_Detached = TRUE;
	msddev->msddev_Detached = TRUE;

	usbbase->usb_IExec->ObtainSemaphore( & msddisk->msddisk_StatSemaphore );

	while(( callback = NODE_REMHEAD( & msddisk->msddisk_MSDUnit_StatList ) ))
	{
		ioreq = callback->Request;

		MEM_FREEVEC( callback );

		ioreq->io_Actual = 0;
		ioreq->io_Error = TDERR_DiskChanged;

		MSGPORT_REPLYMSG( ioreq );
	}

	usbbase->usb_IExec->ReleaseSemaphore( & msddisk->msddisk_StatSemaphore );

	if ( msddisk->msddisk_Announced )
	{
		usbbase->usb_IMounter->DenounceDevice( MSD_Device_Name, msddisk->msddisk_UnitNr );
		msddisk->msddisk_Announced = FALSE;
	}

	// --

	TASK_NAME_LEAVE();
}

// --
