
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

SEC_CODE void
MSD_Device_Free( struct USBBase *usbbase, struct MSDDevice *msddev )
{
	struct MSDBase *msdbase;
	struct MSDDisk *msddisk;

	S32 release;

	if ( ! msddev )
	{
		return;
	}

	TASK_NAME_ENTER( "MSD_Device_Free" );

	USBDEBUG( "MSD : MSD_Device_Free" );

	msdbase = msddev->msddev_MSDBase;

	MSD_Main_2_Stopping( usbbase, msddev );

	while( ( msddisk = NODE_REMHEAD( & msddev->msddev_MSDDisk_Header ) ) )
	{
		MSD_Disk_Free( usbbase, msddisk );
	}

	if ( msddev->msddev_Register )
	{
		REGISTER_UNREGISTER( msddev->msddev_Register );

		msddev->msddev_Register = NULL;
		msddev->msddev_Res_Control = NULL;
		msddev->msddev_Res_Bulk_In = NULL;
		msddev->msddev_Res_Bulk_Out = NULL;
	}

	MSGPORT_FREE( & msddev->msddev_MsgPort_Abort );

	MSGPORT_FREE( & msddev->msddev_MsgPort_Begin );

	SEMAPHORE_OBTAIN( & msdbase->msdbase_MSDDisk_Semaphore );

	release = --msddev->msddev_References == 0;

	SEMAPHORE_RELEASE( & msdbase->msdbase_MSDDisk_Semaphore );

	if ( release )
	{
		MEM_FREEVEC( msddev );
	}

	TASK_NAME_LEAVE();
}

// --
