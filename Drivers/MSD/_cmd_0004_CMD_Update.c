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

SEC_CODE S32 MSD_Cmd_0004_CMD_Update( struct USBBase *usbbase, struct MSDDevice *msddev, struct IOStdReq *ioreq )
{
struct MSDDisk *msddisk;

	TASK_NAME_ENTER( "MSD : CMD_Update" );

	msddisk = (PTR) ioreq->io_Unit;

	if ( ( msddisk ) && ( msddisk->msddisk_Removable ) && ( ! msddisk->msddisk_MediaPresent ) )
	{
		MSD_Disk_MediaCheck( usbbase, msddisk );
	}

	/**/ if ( ( msddev->msddev_Detached ) || ( ! msddisk ) || ( ! msddisk->msddisk_MediaPresent ) )
	{
		ioreq->io_Error = TDERR_DiskChanged;
	}
	else if ( ( msddisk->msddisk_DeviceType != DG_CDROM ) && ( msddev->msddev_SubClass != MSD_SubClass_UFI ) && ( msddev->msddev_SubClass != MSD_SubClass_SFF ) && ( ! MSD_SCSI_SynchronizeCache( usbbase, msddisk ) ) )
	{
		ioreq->io_Error = TDERR_NotSpecified;
	}

	TASK_NAME_LEAVE();

	return ( TRUE );
}

// --
