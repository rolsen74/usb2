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

SEC_CODE S32 MSD_Disk_MediaCheck( struct USBBase *usbbase, struct MSDDisk *msddisk )
{
struct DoBulkResult dbr;
struct DoBulkResult sense_result;
struct IOStdReq *ioreq;
U64 blocks;
U32 size;
U32 shift;
S32 ready;
S32 removed;
S32 changed;
U8 sense[18];

	TASK_NAME_ENTER( "MSD_Disk_MediaCheck" );

	ready = MSD_SCSI_TestUnitReady( usbbase, msddisk, & dbr );
	removed = FALSE;
	changed = FALSE;

	if ( ( ! ready ) && ( dbr.csw_Valid ) && ( dbr.csw_Status == MSDCSW_Status_CmdFailed ) )
	{
		MEM_SET( sense, 0, sizeof( sense ) );

		if ( ( MSD_SCSI_RequestSense( usbbase, msddisk, & sense_result, sense, sizeof( sense ) ) ) && ( sense_result.csw_DataActual >= 14 ) )
		{
			if ( ( ( sense[0] & 0x7f ) == 0x70 ) || ( ( sense[0] & 0x7f ) == 0x71 ) )
			{
				removed = ( ( ( sense[2] & 0x0f ) == 0x02 ) && ( sense[12] == 0x3a ) )
						|| ( ( ( sense[2] & 0x0f ) == 0x06 ) && ( sense[12] == 0x28 ) );
			}
		}
	}

	if ( ( ! ready ) && ( msddisk->msddisk_MSDDev->msddev_Detached ) )
	{
		removed = TRUE;
	}

	if ( ( ! ready ) && ( ! msddisk->msddisk_MediaPresent ) )
	{
		goto bailout;
	}

	if ( ( ready ) && ( ! msddisk->msddisk_MediaPresent ) )
	{
		ready = MSD_SCSI_GetCapacity( usbbase, msddisk, & blocks, & size );

		if ( ready )
		{
			shift = 0;

			while( ( size >> shift ) > 1 )
			{
				shift++;
			}

			msddisk->msddisk_TotalBlocks = blocks;
			msddisk->msddisk_Block_Size = size;
			msddisk->msddisk_Block_Mask = size - 1;
			msddisk->msddisk_Block_Shift = shift;
			msddisk->msddisk_MediaPresent = TRUE;
			msddisk->msddisk_DiskChangeCount++;
			changed = TRUE;
		}
	}
	else if ( ( removed ) && ( msddisk->msddisk_MediaPresent ) )
	{
		msddisk->msddisk_MediaPresent = FALSE;
		msddisk->msddisk_TotalBlocks = 0;
		msddisk->msddisk_Block_Size = 0;
		msddisk->msddisk_Block_Mask = 0;
		msddisk->msddisk_Block_Shift = 0;
		msddisk->msddisk_DiskChangeCount++;
		changed = TRUE;
	}

	if ( changed )
	{
		SEMAPHORE_OBTAIN( & msddisk->msddisk_MSDUnit_Semaphore );

		ioreq = (PTR) msddisk->msddisk_MSDUnit_ChangeList.uh_Head;

		while( ioreq )
		{
			if ( ioreq->io_Data )
			{
				usbbase->usb_IExec->Cause( (PTR) ioreq->io_Data );
			}

			ioreq = Node_Next( ioreq );
		}

		SEMAPHORE_RELEASE( & msddisk->msddisk_MSDUnit_Semaphore );
	}

bailout:
	TASK_NAME_LEAVE();

	return ( ready );
}

// --
