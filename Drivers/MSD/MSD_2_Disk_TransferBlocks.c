
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

SEC_CODE S32
MSD_Disk_TransferBlocks( struct USBBase *usbbase, struct MSDDevice *msddev,
	struct IOStdReq *ioreq, U64 offset, S32 write )
{
	struct DoBulkResult result;
	struct MSDDisk *msddisk;

	U64 block;
	U32 blocks;
	U32 protected;
	U32 actual;
	S32 long_command;
	S32 transferred;

	msddisk = (PTR) ioreq->io_Unit;

	if ( ( ! msddisk ) || ( msddev->msddev_Detached ) )
	{
		ioreq->io_Error = TDERR_DiskChanged;

		return ( TRUE );
	}

	if ( ( msddisk->msddisk_Removable ) && ( ! msddisk->msddisk_MediaPresent ) )
	{
		MSD_Disk_MediaCheck( usbbase, msddisk );

		if ( write )
		{
			ioreq->io_Error = TDERR_DiskChanged;

			return ( TRUE );
		}
	}

	if ( ( ! msddisk->msddisk_MediaPresent ) || ( msddev->msddev_Detached ) )
	{
		ioreq->io_Error = TDERR_DiskChanged;

		return ( TRUE );
	}

	if ( ( ! ioreq->io_Data ) || ( ! ioreq->io_Length ) || ( ! msddisk->msddisk_Block_Size ) ||
		( ( offset | ioreq->io_Length ) & msddisk->msddisk_Block_Mask ) )
	{
		ioreq->io_Error = IOERR_BADLENGTH;

		return ( TRUE );
	}

	if ( write )
	{
		if ( ( msddisk->msddisk_DeviceType == DG_CDROM ) ||
			( ( msddisk->msddisk_Removable ) && ( MSD_SCSI_GetWriteProtect( usbbase, msddisk, & protected ) ) && ( protected ) ) )
		{
			ioreq->io_Error = TDERR_WriteProt;

			return ( TRUE );
		}

		if ( ( ! msddisk->msddisk_MediaPresent ) || ( msddev->msddev_Detached ) )
		{
			ioreq->io_Error = TDERR_DiskChanged;

			return ( TRUE );
		}
	}

	block = offset >> msddisk->msddisk_Block_Shift;
	blocks = ioreq->io_Length >> msddisk->msddisk_Block_Shift;

	if ( ( block >= msddisk->msddisk_TotalBlocks ) || ( blocks > msddisk->msddisk_TotalBlocks - block ) )
	{
		ioreq->io_Error = IOERR_BADADDRESS;

		return ( TRUE );
	}

	long_command = ( block > 0xffffffffULL ) || ( blocks - 1 > 0xffffffffULL - block );

	if ( ( long_command ) && ( msddev->msddev_SubClass != MSD_SubClass_SCSI ) )
	{
		ioreq->io_Error = IOERR_BADADDRESS;

		return ( TRUE );
	}

	actual = 0;

	MEM_SET( & result, 0, sizeof( result ) );

	if ( write )
	{
		if ( long_command )
		{
			transferred = MSD_SCSI_Write16( usbbase, msddisk, & result, block, blocks, ioreq->io_Data, & actual );
		}
		else
		{
			transferred = MSD_SCSI_Write10( usbbase, msddisk, & result, block, blocks, ioreq->io_Data, & actual );
		}
	}
	else if ( long_command )
	{
		transferred = MSD_SCSI_Read16( usbbase, msddisk, & result, block, blocks, ioreq->io_Data, & actual );
	}
	else
	{
		transferred = MSD_SCSI_Read10( usbbase, msddisk, & result, block, blocks, ioreq->io_Data, & actual );
	}

	ioreq->io_Actual = actual;

	if ( ! transferred )
	{
		ioreq->io_Error = MSD_Disk_CommandError( usbbase, msddisk, & result );
	}

	return ( TRUE );
}

// --
