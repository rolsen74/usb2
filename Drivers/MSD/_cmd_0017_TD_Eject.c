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

SEC_CODE S32 MSD_Cmd_0017_TD_Eject( struct USBBase *usbbase, struct MSDDevice *msddev, struct IOStdReq *ioreq )
{
struct DoBulkStruct dbs;
struct MSDDisk *msddisk;

	TASK_NAME_ENTER( "MSD : TD_Eject" );

	msddisk = (PTR) ioreq->io_Unit;
	ioreq->io_Actual = 0;

	if ( ( msddev->msddev_Detached ) || ( ! msddisk ) )
	{
		ioreq->io_Error = TDERR_DiskChanged;
		goto bailout;
	}

	if ( ! msddisk->msddisk_Removable )
	{
		ioreq->io_Error = IOERR_NOCMD;
		goto bailout;
	}

	if ( ioreq->io_Length > 1 )
	{
		ioreq->io_Error = IOERR_BADLENGTH;
		goto bailout;
	}

	MEM_SET( & dbs, 0, sizeof( dbs ) );
	dbs.dbs_Command_Length = 6;
	dbs.cmd[0] = 0x1b;
	dbs.cmd[4] = ( ioreq->io_Length ) ? 0x02 : 0x03;

	if ( ! MSD_Disk_DoBulk( usbbase, msddisk, & dbs ) )
	{
		ioreq->io_Error = ( msddev->msddev_Detached ) ? TDERR_DiskChanged : TDERR_NotSpecified;
		goto bailout;
	}

	MSD_Disk_MediaCheck( usbbase, msddisk );
	ioreq->io_Error = 0;

bailout:
	TASK_NAME_LEAVE();

	return ( TRUE );
}

// --
