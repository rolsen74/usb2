
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

SEC_CODE struct MSDDisk *MSD_Disk_Alloc( struct USBBase *usbbase, struct MSDDevice *msddev, U32 lun )
{
struct MSDBase *msdbase;
struct MSDDisk *msddisk;
U32 blk_size;
U64 blk_secs;
U32 unit_nr;
S32 error;
S32 stat;
U32 cnt;
U8 inquiry[36];
U8 type;

	TASK_NAME_ENTER( "MSD_Disk_Alloc" );
	USBERROR( "MSD : MSD_Disk_Alloc" );

	// ---

	error = TRUE;

	msdbase = usbbase->usb_MSDDevice;
	msddisk = MEM_ALLOCVEC( sizeof( struct MSDDisk ), TRUE );

	if ( ! msddisk )
	{
		USBERROR( "MSD : MSD_Disk_Alloc : Error allocating memory" );
		goto bailout;
	}

	usbbase->usb_IExec->InitSemaphore( & msddisk->msddisk_StatSemaphore );

	SEMAPHORE_INIT( & msddisk->msddisk_MSDUnit_Semaphore );

	msddisk->msddisk_Lun = lun;
	msddisk->msddisk_MSDDev = msddev;
	msddisk->msddisk_MSDBase = msdbase;
	msddisk->msddisk_GeoChangeCount = -1;
	msddisk->msddisk_UnitNr = MAX_MSD_UNITS;

	// --
	// Inquire MSD Type

	for( cnt=0 ; cnt<3 ; cnt++ )
	{
		USBERROR( "MSD : MSD_Disk_Alloc : MSD_SCSI_Inquiry : Lun #%lu : Cnt #%lu", lun, cnt );

		if ( MSD_SCSI_Inquiry( usbbase, msddisk, inquiry, 36 ))
		{
			USBERROR( "MSD : MSD_Disk_Alloc : MSD_SCSI_Inquiry : #%ld : Okay", cnt );
			break;
		}
		else
		{
			USBERROR( "MSD : MSD_Disk_Alloc : MSD_SCSI_Inquiry : Try #%ld/3 : Failed", cnt+1 );

			MISC_MICRODELAY( 100 * 1000 );		// Wait 100ms
		}
	}

	if ( cnt >= 3 )
	{
		// No responce, proberly nobody there
		USBERROR( "MSD : MSD_Disk_Alloc : No reponce for Lun #%lu", lun );
		goto bailout;
	}

	// --
	// Check MSD Type

	type = inquiry[0] & 0x1F;

	/**/ if ( type == DG_DIRECT_ACCESS )
	{
		USBERROR( "MSD : MSD_Disk_Alloc : Found new 'Direct Access' device" );
	}
	else if ( type == DG_OPTICAL_DISK )
	{
		USBERROR( "MSD : MSD_Disk_Alloc : Found new 'Optical Disk' device" );
	}
	else if ( type == DG_CDROM )
	{
		USBERROR( "MSD : MSD_Disk_Alloc : Found new 'CDRom' device" );
	}
	else
	{
		USBERROR( "MSD : MSD_Disk_Alloc : Unsupported Device Type (%lu)", type );
		goto bailout;
	}

	// Some USB bridges omit the removable-media bit for CD/DVD drives.
	msddisk->msddisk_DeviceType	= type;
	msddisk->msddisk_Removable	= ( ( type == DG_CDROM ) || ( inquiry[1] & 0x80 ) ) ? TRUE : FALSE;

	// --
	// We need to execute a Test Unit Ready .. too turn on the device

	if ( ( ! msddisk->msddisk_Removable ) && ( ! MSD_Disk_TestUnitReady( usbbase, msddisk ) ) )
	{
		USBERROR( "MSD : MSD_Disk_Alloc : MSD_Disk_TestUnitReady() failed" );
		goto bailout;
	}

	// --
	// Get Block Size

	if ( msddisk->msddisk_Removable )
	{
		stat = MSD_Disk_MediaCheck( usbbase, msddisk );

		if ( msddev->msddev_Detached )
		{
			goto bailout;
		}
	}
	else
	{
		stat = MSD_SCSI_GetCapacity( usbbase, msddisk, & blk_secs, & blk_size );
	}

	if ( ( ! stat ) && ( ! msddisk->msddisk_Removable ) )
	{
		USBERROR( "MSD : MSD_Disk_Parse : MSD_SCSI_GetCapacity() failed" );
		goto bailout;
	}

	// Mask and Shift is too avoid 64bit compiler __umoddi3 and __udivi3 functions
	if ( ! msddisk->msddisk_Removable )
	{
		msddisk->msddisk_TotalBlocks = blk_secs;
		msddisk->msddisk_Block_Size = blk_size;
		msddisk->msddisk_Block_Mask = blk_size - 1;
		msddisk->msddisk_Block_Shift = 0;
		msddisk->msddisk_MediaPresent = TRUE;

		while( blk_size > 1 )
		{
			msddisk->msddisk_Block_Shift++;
			blk_size >>= 1;
		}
	}

	USBERROR( "msddisk_Block_Size ... : %lu",  msddisk->msddisk_Block_Size );
	USBERROR( "msddisk_Block_Mask ... : %lu",  msddisk->msddisk_Block_Mask );
	USBERROR( "msddisk_Block_Shift .. : %lu",  msddisk->msddisk_Block_Shift );
	USBERROR( "msddisk_TotalBlocks .. : %llu", msddisk->msddisk_TotalBlocks );

	// --
	// Mount

	msddisk->msddisk_MsgPort_Abort = & msddev->msddev_MsgPort_Abort;
	msddisk->msddisk_MsgPort_Begin = & msddev->msddev_MsgPort_Begin;

	// --
	// End Init with, find Unit slot and then Add/Mount

	SEMAPHORE_OBTAIN( & msdbase->msdbase_MSDDisk_Semaphore );

	for( unit_nr=0 ; unit_nr<MAX_MSD_UNITS ; unit_nr++ )
	{
		if ( ! msdbase->msdbase_MSDDisk_Units[ unit_nr ] )
		{
			msddisk->msddisk_UnitNr = unit_nr;
			msdbase->msdbase_MSDDisk_Units[ unit_nr ] = msddisk;
			break;
		}
	}

	SEMAPHORE_RELEASE( & msdbase->msdbase_MSDDisk_Semaphore );

	if ( unit_nr >= MAX_MSD_UNITS )
	{
		USBERROR( "MSD : MSD_Unit_Alloc : No free Unit slots" );
		goto bailout;
	}

	// --

	stat = usbbase->usb_IMounter->AnnounceDeviceTags( MSD_Device_Name, msddisk->msddisk_UnitNr,
		MNTA_DosNamePrefixHint, "USB",
		TAG_END
	);

	msddisk->msddisk_Announced = ( stat ) ? TRUE : FALSE ;
	USBERROR( "Mount Stat: %ld (%s/%ld)", stat, "USB", msddisk->msddisk_UnitNr );

	MSDDEBUG( "MSD: unit %lu %04lX:%04lX LUN %lu, media %lu, mounter announce %ld\n",
		unit_nr, (U32) msddev->msddev_StartMsg->VendorID, (U32) msddev->msddev_StartMsg->DeviceID,
		lun, (U32) msddisk->msddisk_MediaPresent, stat );

	// --

	error = FALSE;

	USBERROR( "hmm disk okay" );

bailout:

	if (( msddisk ) && ( error ))
	{
		MSD_Disk_Free( usbbase, msddisk );
		msddisk = NULL;
	}

	TASK_NAME_LEAVE();
	return( msddisk );
}

// --
