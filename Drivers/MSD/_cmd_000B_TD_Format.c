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

SEC_CODE S32 MSD_Cmd_000B_TD_Format( struct USBBase *usbbase, struct MSDDevice *msddev, struct IOStdReq *ioreq )
{
struct MSDDisk *msddisk;
U64 block;
U32 blocks;
U32 actual;
U32 protected;
S32 stat;

	TASK_NAME_ENTER( "MSD : TD_Format" );

	msddisk = (PTR) ioreq->io_Unit;
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

	if ( msddisk->msddisk_DeviceType == DG_CDROM )
	{
		ioreq->io_Error = TDERR_WriteProt;
		goto bailout;
	}

	if ( ( msddisk->msddisk_Removable ) && ( MSD_SCSI_GetWriteProtect( usbbase, msddisk, & protected ) ) && ( protected ) )
	{
		ioreq->io_Error = TDERR_WriteProt;
		goto bailout;
	}

	if ( ( ! msddisk->msddisk_Block_Size ) || ( ! ioreq->io_Data ) || ( ! ioreq->io_Length ) )
	{
		ioreq->io_Error = IOERR_BADLENGTH;
		goto bailout;
	}

	if ( ( ioreq->io_Offset & msddisk->msddisk_Block_Mask ) || ( ioreq->io_Length & msddisk->msddisk_Block_Mask ) )
	{
		ioreq->io_Error = IOERR_BADLENGTH;
		goto bailout;
	}

	block = (U64) ioreq->io_Offset >> msddisk->msddisk_Block_Shift;
	blocks = ioreq->io_Length >> msddisk->msddisk_Block_Shift;

	if ( ( block >= msddisk->msddisk_TotalBlocks ) || ( blocks > msddisk->msddisk_TotalBlocks - block ) )
	{
		ioreq->io_Error = IOERR_BADADDRESS;
		goto bailout;
	}

	// Trackdisk format supplies the sector data; it does not request SCSI FORMAT UNIT.
	actual = 0;

	if ( ( block > 0xffffffffULL ) || ( blocks - 1 > 0xffffffffULL - block ) )
	{
		stat = MSD_SCSI_Write16( usbbase, msddisk, NULL, block, blocks, ioreq->io_Data, & actual );
	}
	else
	{
		stat = MSD_SCSI_Write10( usbbase, msddisk, NULL, (U32) block, blocks, ioreq->io_Data, & actual );
	}

	ioreq->io_Actual = actual;

	if ( ! stat )
	{
		ioreq->io_Error = ( msddev->msddev_Detached ) ? TDERR_DiskChanged : TDERR_NotSpecified;
	}

bailout:
	TASK_NAME_LEAVE();

	return ( TRUE );
}

// --
