
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

SEC_CODE S32 MSD_Cmd_0016_TD_GetGeometry( struct USBBase *usbbase, struct MSDDevice *msddev UNUSED, struct IOStdReq *ioreq )
{
struct DriveGeometry tmp_dg;
struct MSDDisk *msddisk;
U32 blk_size;	// Block size 512/2048
U32 tmp_size;
U64 blk_secs;	// Total Sectors
U32 actual;
S32 reply;

	TASK_NAME_ENTER( "MSD : TD_GetGeometry" );
	USBERROR( "MSD : _cmd_0016_TD_GetGeometry" );

	reply = TRUE;
	msddisk = (PTR) ioreq->io_Unit;

	if ( msddisk->msddisk_Removable )
	{
		MSD_Disk_MediaCheck( usbbase, msddisk );
	}

	if ( ( ! msddisk->msddisk_MediaPresent ) || ( ! msddisk->msddisk_Block_Size ) )
	{
		ioreq->io_Error = TDERR_DiskChanged;
		ioreq->io_Actual = 0;
		goto bailout;
	}


	if (( ! ioreq->io_Data ) || ( ! ioreq->io_Length ))
	{
		ioreq->io_Error = IOERR_BADLENGTH;
		USBERROR( "MSD : NULL Pointer" );
		goto bailout;
	}

	actual = MIN( ioreq->io_Length, sizeof( struct DriveGeometry ));

	#if 0
	// If unchanged, return cached geometry
	if ( msddisk->msddisk_GeoChangeCount == msddisk->msddisk_DiskChangeCount )
	{
		USBERROR( "MSD : No change" );
		MEM_COPY( & msddisk->msddisk_Geometry, ioreq->io_Data, actual );
		ioreq->io_Actual = actual;
		ioreq->io_Error = 0;
		goto bailout;
	}
	#endif

	MEM_SET( & tmp_dg, 0, sizeof( struct DriveGeometry ));

	if ( ! MSD_SCSI_GetCapacity( usbbase, msddisk, & blk_secs, & blk_size ) )
	{
		USBERROR( "MSD : MSD_SCSI_GetCapacity() failed" );
		ioreq->io_Error = TDERR_NotSpecified;
		goto bailout;
	}

	// tmp setting, as I do no calc.. 
	// proberly only good for small storages devices 
	tmp_dg.dg_SectorSize	= (U32) blk_size;
	tmp_dg.dg_TotalSectors	= ( blk_secs > 0xffffffffULL ) ? 0xffffffffUL : (U32) blk_secs;
	tmp_dg.dg_Cylinders		= tmp_dg.dg_TotalSectors;
	tmp_dg.dg_CylSectors	= 1;
	tmp_dg.dg_Heads			= 1;
	tmp_dg.dg_TrackSectors	= 1;
	tmp_dg.dg_BufMemType	= MEMF_PUBLIC;
	tmp_dg.dg_DeviceType	= msddisk->msddisk_DeviceType;
	tmp_dg.dg_Flags			= ( msddisk->msddisk_Removable ) ? DGF_REMOVABLE : 0 ;
	tmp_dg.dg_Reserved		= 0;

	// We are okay now

	// Update Internal Buffers
	MEM_COPY( & tmp_dg, & msddisk->msddisk_Geometry, sizeof( struct DriveGeometry ));
	msddisk->msddisk_GeoChangeCount = msddisk->msddisk_DiskChangeCount;
//	msddisk->msddisk_Block_Size = (U32) blk_size;
	msddisk->msddisk_TotalBlocks = blk_secs;

	// Mask and Shift is too avoid 64bit compiler __umoddi3 and __udivi3 functions
	msddisk->msddisk_Block_Size = blk_size;
	msddisk->msddisk_Block_Mask = blk_size - 1;
	msddisk->msddisk_Block_Shift = 0;

	tmp_size = blk_size;
	while( tmp_size > 1 )
	{
		msddisk->msddisk_Block_Shift++;
		tmp_size >>= 1;
	}

	// --
	// Update User Buffers
	MEM_COPY( & msddisk->msddisk_Geometry, ioreq->io_Data, actual );
	ioreq->io_Actual = actual;
	ioreq->io_Error = 0;

	MSDDEBUG( "MSD: unit %lu geometry, %lu sectors of %lu bytes\n",
		msddisk->msddisk_UnitNr, tmp_dg.dg_TotalSectors, tmp_dg.dg_SectorSize );

bailout:

	TASK_NAME_LEAVE();
	return( reply );
}

#endif
// --
