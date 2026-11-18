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

SEC_CODE S32 MSD_Cmd_000A_TD_Seek( struct USBBase *usbbase UNUSED, struct MSDDevice *msddev, struct IOStdReq *ioreq )
{
struct MSDDisk *msddisk;
U64 block;
U32 offset;

	TASK_NAME_ENTER( "MSD : TD_Seek" );

	msddisk = (PTR) ioreq->io_Unit;
	offset = ioreq->io_Offset;
	ioreq->io_Actual = 0;

	if ( ( msddev->msddev_Detached ) || ( ! msddisk ) )
	{
		ioreq->io_Error = TDERR_DiskChanged;
		goto bailout;
	}

	if ( ( msddisk->msddisk_Removable ) && ( ! msddisk->msddisk_MediaPresent ) )
	{
		MSD_Disk_MediaCheck( usbbase, msddisk );
	}

	if ( ! msddisk->msddisk_MediaPresent )
	{
		ioreq->io_Error = TDERR_DiskChanged;
		goto bailout;
	}

	if ( ! msddisk->msddisk_Block_Size )
	{
		ioreq->io_Error = TDERR_NotSpecified;
		goto bailout;
	}

	if ( offset & msddisk->msddisk_Block_Mask )
	{
		ioreq->io_Error = IOERR_BADLENGTH;
		goto bailout;
	}

	block = (U64) offset >> msddisk->msddisk_Block_Shift;

	if ( block >= msddisk->msddisk_TotalBlocks )
	{
		ioreq->io_Error = IOERR_BADADDRESS;
		goto bailout;
	}

	// Block transfers carry their own address, so there is no device position to update.
	ioreq->io_Error = 0;

bailout:
	TASK_NAME_LEAVE();

	return ( TRUE );
}

// --
