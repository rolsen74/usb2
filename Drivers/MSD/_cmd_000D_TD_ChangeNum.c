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

SEC_CODE S32 MSD_Cmd_000D_TD_ChangeNum( struct USBBase *usbbase, struct MSDDevice *msddev, struct IOStdReq *ioreq )
{
struct MSDDisk *msddisk;

	TASK_NAME_ENTER( "MSD : TD_ChangeNum" );

	msddisk = (PTR) ioreq->io_Unit;

	if ( ( ! msddisk ) || ( msddev->msddev_Detached ) )
	{
		ioreq->io_Error = TDERR_DiskChanged;
		ioreq->io_Actual = 0;
	}
	else
	{
		if ( msddisk->msddisk_Removable )
		{
			MSD_Disk_MediaCheck( usbbase, msddisk );
		}

		ioreq->io_Actual = msddisk->msddisk_DiskChangeCount;
		ioreq->io_Error = 0;
	}

	TASK_NAME_LEAVE();

	return ( TRUE );
}

// --
