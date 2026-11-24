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
MSD_Disk_MediaCheck( struct USBBase *usbbase, struct MSDDisk *msddisk )
{
	struct DoBulkResult result;

	U64 blocks;
	U32 size;
	U32 shift;
	S32 ready;

#ifdef MSD_DEBUG
	if ( ( MSD_TRACE_OPTICAL( msddisk ) ) && ( ! msddisk->msddisk_ReadyTrace.Seen ) )
	{
		MSDDEBUG( "MSD: interface %lu LUN %lu starting TEST UNIT READY\n",
			msddisk->msddisk_MSDDev->msddev_InterfaceNr, msddisk->msddisk_Lun );
	}
#endif

	ready = MSD_SCSI_TestUnitReady( usbbase, msddisk, &result );

#ifdef MSD_DEBUG
	if ( ( MSD_TRACE_OPTICAL( msddisk ) ) &&
		( MSD_TraceDue( &msddisk->msddisk_ReadyTrace,
			( (U32) ready << 24 ) | ( (U32) result.CommandSent << 16 ) |
			( (U32) result.csw_Valid << 8 ) | result.csw_Status ) ) )
	{
		if ( result.csw_Valid )
		{
			MSDDEBUG( "MSD: interface %lu LUN %lu TEST UNIT READY %s, CSW status $%02lX\n",
				msddisk->msddisk_MSDDev->msddev_InterfaceNr, msddisk->msddisk_Lun,
				( ready ) ? "ready" : "failed", (U32) result.csw_Status );
		}
		else
		{
			MSDDEBUG( "MSD: interface %lu LUN %lu TEST UNIT READY, no valid CSW, CBW sent %lu\n",
				msddisk->msddisk_MSDDev->msddev_InterfaceNr, msddisk->msddisk_Lun,
				(U32) result.CommandSent );
		}
	}
#endif

	if ( ! ready )
	{
		MSD_Disk_CommandError( usbbase, msddisk, &result );
	}

	if ( ( msddisk->msddisk_MSDDev->msddev_Detached ) || ( msddisk->msddisk_Block_Size ) )
	{
		return ( ready );
	}

	if ( ( ready ) && ( MSD_SCSI_GetCapacity( usbbase, msddisk, &blocks, &size ) ) )
	{
		shift = 0;

		while ( ( size >> shift ) > 1 )
		{
			shift++;
		}

		msddisk->msddisk_TotalBlocks = blocks;
		msddisk->msddisk_Block_Size = size;
		msddisk->msddisk_Block_Mask = size - 1;
		msddisk->msddisk_Block_Shift = shift;
	}
	else if ( ( msddisk->msddisk_MSDDev->msddev_SubClass != MSD_SubClass_UFI ) ||
		( msddisk->msddisk_MSDDev->msddev_Detached ) ||
		( ! MSD_SCSI_UnformattedMedia( usbbase, msddisk ) ) )
	{
		return ( FALSE );
	}

	if ( ! msddisk->msddisk_MediaPresent )
	{
		msddisk->msddisk_MediaPresent = TRUE;
		msddisk->msddisk_DiskChangeCount++;

		MSD_Disk_CancelWrites( usbbase, msddisk );

		MSD_Disk_NotifyChange( usbbase, msddisk );
	}

	return ( TRUE );
}

// --
