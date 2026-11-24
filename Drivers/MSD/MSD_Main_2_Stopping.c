
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
MSD_Main_2_Stopping( struct USBBase *usbbase, struct MSDDevice *msddev )
{
	struct MSDBase *msdbase;
	struct MSDDisk *msddisk;
	struct IOStdReq *ioreq;

	msdbase = msddev->msddev_MSDBase;

	SEMAPHORE_OBTAIN( & msdbase->msdbase_MSDDisk_Semaphore );

	msddev->Running = FALSE;
	msddev->msddev_Detached = TRUE;

	for( msddisk = Header_Head( & msddev->msddev_MSDDisk_Header ); msddisk; msddisk = Node_Next( msddisk ) )
	{
		msddisk->msddisk_MsgPort_Begin = NULL;
	}

	SEMAPHORE_RELEASE( & msdbase->msdbase_MSDDisk_Semaphore );

	if ( msddev->msddev_MsgPort_Begin.ump_StructID == ID_USB2_MP )
	{
		while( ( ioreq = MSGPORT_GETMSG( & msddev->msddev_MsgPort_Begin ) ) )
		{
			ioreq->io_Actual = 0;
			ioreq->io_Error = TDERR_DiskChanged;

			MSGPORT_REPLYMSG( ioreq );
		}
	}

	for( msddisk = Header_Head( & msddev->msddev_MSDDisk_Header ); msddisk; msddisk = Node_Next( msddisk ) )
	{
		MSD_Disk_Detach( usbbase, msddisk );
	}
}

// --
