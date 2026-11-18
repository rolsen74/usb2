
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
#if 1

SEC_CODE S32 MSD_Cmd_0012_TD_GetDriveType( struct USBBase *usbbase UNUSED, struct MSDDevice *msddev, struct IOStdReq *ioreq )
{
struct MSDDisk *msddisk;

	TASK_NAME_ENTER( "MSD : TD_GetDriveType" );

	msddisk = (PTR) ioreq->io_Unit;
	ioreq->io_Actual = 0;

	if ( ( msddev->msddev_Detached ) || ( ! msddisk ) )
	{
		ioreq->io_Error = TDERR_DiskChanged;
	}
	else if ( msddev->msddev_SubClass == MSD_SubClass_UFI )
	{
		ioreq->io_Actual = DRIVE3_5;
		ioreq->io_Error = 0;
	}
	else
	{
		// Drive type codes describe floppy drives; use TD_GETGEOMETRY for disks and CDs.
		ioreq->io_Error = IOERR_NOCMD;
	}

	TASK_NAME_LEAVE();

	return ( TRUE );
}

#endif
// --
