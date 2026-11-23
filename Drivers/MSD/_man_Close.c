
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

SEC_CODE PTR MSD_Manager_Close( struct DeviceManagerInterface *Self, struct IORequest *ioreq )
{
struct USBBase *usbbase;
struct MSDBase *MSDBase;
struct MSDDisk *msddisk;
struct MSDDevice *msddev;

S32 free_disk;
S32 free_dev;

	MSDBase = (PTR) Self->Data.LibBase;
	usbbase = MSDBase->msdbase_usbbase;

	TASK_NAME_ENTER( "MSD : MSD_Manager_Close" );
	USBERROR( "MSD : MSD_Manager_Close" );

	SEMAPHORE_OBTAIN( & MSDBase->msdbase_MSDDisk_Semaphore );

	// Adjust usb2disk.device open count
	MSDBase->msdbase_Library.lib_OpenCnt--;

	// Adjust msd/unit open count
	msddisk = (PTR) ioreq->io_Unit;
	msddev = msddisk->msddisk_MSDDev;
	msddisk->msddisk_OpenCnt--;
	free_disk = ( msddisk->msddisk_Released ) && ( ! msddisk->msddisk_OpenCnt );
	free_dev = --msddev->msddev_References == 0;

	// Make sure IOReq will not be used again
	ioreq->io_Unit	 = NULL;
	ioreq->io_Device = NULL;

	SEMAPHORE_RELEASE( & MSDBase->msdbase_MSDDisk_Semaphore );

	if ( free_disk )
	{
		MEM_FREEVEC( msddisk );
	}

	if ( free_dev )
	{
		MEM_FREEVEC( msddev );
	}

	TASK_NAME_LEAVE();
	return( NULL );
}

// --
