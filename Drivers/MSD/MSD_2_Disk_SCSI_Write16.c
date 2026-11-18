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

SEC_CODE S32 MSD_SCSI_Write16(
	struct USBBase *usbbase,
	struct MSDDisk *msddisk,
	struct DoBulkResult *dbr,
	U64 blk_start,
	U32 blk_blocks,
	PTR mem,
	U32 *actual_ptr )
{
struct DoBulkResult dbr_dummy;
struct DoBulkStruct dbs;
U32 max_blocks;
U32 got_blocks;
U32 got_bytes;
U32 blk_size;
U32 remaining;
U32 blocks;
U32 actual;
U32 len;
U64 start;
S32 retval;

	TASK_NAME_ENTER( "MSD_SCSI_Write16" );

	actual = 0;
	retval = FALSE;

	if ( ! dbr )
	{
		dbr = & dbr_dummy;
	}

	blk_size = msddisk->msddisk_Block_Size;

	if ( ( ! blk_size ) || ( blk_size & ( blk_size - 1 ) ) )
	{
		USBERROR( "MSD_SCSI_Write16 : Invalid block size %lu", blk_size );
		goto bailout;
	}

	max_blocks = ( 5 * 20 * 1024 ) / blk_size;
	max_blocks = MAX( max_blocks, 1 );
	max_blocks = MIN( max_blocks, 0xffff );
	remaining = blk_blocks;
	start = blk_start;

	MEM_SET( & dbs, 0, sizeof( dbs ) );
	dbs.dbs_Flags = DOBULKF_DIR_OUT;
	dbs.dbs_Result = dbr;
	dbs.dbs_Command_Length = 16;
	dbs.cmd[0] = SCSI_DA_WRITE_16;

	while( remaining )
	{
		blocks = MIN( max_blocks, remaining );
		len = blocks * blk_size;

		dbs.dbs_Data = mem;
		dbs.dbs_Data_Length = len;
		dbs.cmd[2] = ( start >> 56 ) & 0xff;
		dbs.cmd[3] = ( start >> 48 ) & 0xff;
		dbs.cmd[4] = ( start >> 40 ) & 0xff;
		dbs.cmd[5] = ( start >> 32 ) & 0xff;
		dbs.cmd[6] = ( start >> 24 ) & 0xff;
		dbs.cmd[7] = ( start >> 16 ) & 0xff;
		dbs.cmd[8] = ( start >> 8 ) & 0xff;
		dbs.cmd[9] = start & 0xff;
		dbs.cmd[10] = ( blocks >> 24 ) & 0xff;
		dbs.cmd[11] = ( blocks >> 16 ) & 0xff;
		dbs.cmd[12] = ( blocks >> 8 ) & 0xff;
		dbs.cmd[13] = blocks & 0xff;

		if ( ! MSD_Disk_DoBulk( usbbase, msddisk, & dbs ) )
		{
			USBERROR( "MSD_SCSI_Write16 : Transfer failed" );
			goto bailout;
		}

		if ( ( ! dbr->csw_Valid ) || ( dbr->csw_Status != MSDCSW_Status_CmdPassed ) || ( dbr->csw_DataActual != len ) )
		{
			USBERROR( "MSD_SCSI_Write16 : Incomplete transfer" );
			goto bailout;
		}

		got_bytes = dbr->csw_DataActual;

		if ( got_bytes & ( blk_size - 1 ) )
		{
			USBERROR( "MSD_SCSI_Write16 : Partial block" );
			goto bailout;
		}

		got_blocks = got_bytes / blk_size;
		mem = (U8 *) mem + got_bytes;
		start += got_blocks;
		actual += got_bytes;
		remaining -= got_blocks;
	}

	retval = TRUE;

bailout:

	if ( actual_ptr )
	{
		*actual_ptr = actual;
	}

	TASK_NAME_LEAVE();

	return ( retval );
}

// --
