
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

SEC_CODE S32 MSD_Cmd_000F_TD_ProtStatus( struct USBBase *usbbase, struct MSDDevice *msddev, struct IOStdReq *ioreq )
{
struct MSDDisk *msddisk;
U32 protected;
S32 reply;

	TASK_NAME_ENTER( "MSD : _cmd_000F_TD_ProtStatus" );
	USBERROR( "MSD : _cmd_000F_TD_ProtStatus" );

	#if 0
	io_Actual - if io_Error is 0, this tells you whether the disk in the
				drive is write-protected. 0 means the disk is NOT write-
				protected, while any other value indicates it is.
	#endif

	msddisk = (PTR) ioreq->io_Unit;
	protected = TRUE;

	if ( ( msddisk ) && ( msddisk->msddisk_Removable ) && ( ! msddev->msddev_Detached ) )
	{
		MSD_Disk_MediaCheck( usbbase, msddisk );
	}

	/**/ if ( ( ! msddisk ) || ( msddev->msddev_Detached ) || ( ! msddisk->msddisk_MediaPresent ) )
	{
		ioreq->io_Error = TDERR_DiskChanged;
	}
	else if ( msddisk->msddisk_DeviceType == DG_CDROM )
	{
		ioreq->io_Error = 0;
	}
	else if ( MSD_SCSI_GetWriteProtect( usbbase, msddisk, & protected ) )
	{
		ioreq->io_Error = 0;
	}
	else
	{
		ioreq->io_Error = TDERR_NotSpecified;
	}

	ioreq->io_Actual = protected;
	reply = TRUE;

	TASK_NAME_LEAVE();
	return( reply );
}

#endif
// --
