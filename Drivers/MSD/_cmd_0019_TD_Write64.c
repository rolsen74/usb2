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

SEC_CODE S32 MSD_Cmd_0019_TD_Write64( struct USBBase *usbbase, struct MSDDevice *msddev, struct IOStdReq *ioreq )
{
struct MSDDisk *msddisk;
U64 blk_start;
U64 offset;
U32 blk_blocks;
U32 blk_mask;
U32 actual;
U32 protected;
S32 stat;

	TASK_NAME_ENTER( "MSD : TD_Write64" );

	msddisk = (PTR) ioreq->io_Unit;
	offset = ( (U64) ioreq->io_Actual << 32 ) | ioreq->io_Offset;
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

	blk_mask = msddisk->msddisk_Block_Mask;

	if ( ( offset & blk_mask ) || ( ioreq->io_Length & blk_mask ) )
	{
		USBERROR( "MSD : TD_Write64 : Unaligned request" );
		ioreq->io_Error = IOERR_BADLENGTH;
		goto bailout;
	}

	blk_start = offset >> msddisk->msddisk_Block_Shift;
	blk_blocks = ioreq->io_Length >> msddisk->msddisk_Block_Shift;

	if ( ( blk_start >= msddisk->msddisk_TotalBlocks ) || ( blk_blocks > msddisk->msddisk_TotalBlocks - blk_start ) )
	{
		USBERROR( "MSD : TD_Write64 : Request exceeds disk capacity" );
		ioreq->io_Error = IOERR_BADLENGTH;
		goto bailout;
	}

	actual = 0;

	if ( ( blk_start > 0xffffffffULL ) || ( blk_blocks - 1 > 0xffffffffULL - blk_start ) )
	{
		stat = MSD_SCSI_Write16( usbbase, msddisk, NULL, blk_start, blk_blocks, ioreq->io_Data, & actual );
	}
	else
	{
		stat = MSD_SCSI_Write10( usbbase, msddisk, NULL, (U32) blk_start, blk_blocks, ioreq->io_Data, & actual );
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
