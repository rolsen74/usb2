
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

SEC_CODE S32 MSD_Cmd_000E_TD_ChangeState( struct USBBase *usbbase, struct MSDDevice *msddev, struct IOStdReq *ioreq )
{
struct MSDDisk *msddisk;
S32 reply;

	TASK_NAME_ENTER( "MSD : _cmd_000E_TD_ChangeState" );
	USBERROR( "MSD : _cmd_000E_TD_ChangeState" );

	#if 0
	io_Actual - if io_Error is 0, this tells you whether a disk is in
				the drive. 0 means there is a disk, while anything else
				indicates there is no disk
	#endif

	msddisk = (PTR) ioreq->io_Unit;

	if ( ( msddisk ) && ( msddisk->msddisk_Removable ) && ( ! msddev->msddev_Detached ) )
	{
		MSD_Disk_MediaCheck( usbbase, msddisk );
	}

	// 0 = media present, 1 = no media
	ioreq->io_Actual = ( ( msddev->msddev_Detached ) || ( ! msddisk ) || ( ! msddisk->msddisk_MediaPresent ) ) ? TRUE : FALSE;
	ioreq->io_Error = 0;
	reply = TRUE;

	TASK_NAME_LEAVE();
	return( reply );
}

#endif
// --
