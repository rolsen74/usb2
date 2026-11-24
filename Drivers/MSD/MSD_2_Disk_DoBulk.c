
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
MSD_Disk_DoBulk( struct USBBase *usbbase, struct MSDDisk *msddisk, struct DoBulkStruct *dbs )
{
	struct DoBulkResult *dbr;
	struct MSDDevice *msddev;
	enum PhaseStat stat;

	U32 residue;
	U32 actual;
	U32 status;
	S32 allow_short;
	S32 retval;

	TASK_NAME_ENTER( "MSD_Disk_DoBulk" );

	msddev = msddisk->msddisk_MSDDev;
	dbr = dbs->dbs_Result;
	retval = FALSE;
	dbs->dbs_Data_Actual = 0;
	allow_short = ( ( dbs->dbs_Flags & DOBULKF_DIR_MASK ) == DOBULKF_DIR_IN ) &&
		( dbs->dbs_Flags & DOBULKF_SHORT_OK );

	if ( dbr )
	{
		MEM_SET( dbr, 0, sizeof( *dbr ) );
	}

	if ( ( ! dbs->dbs_Command_Length ) || ( dbs->dbs_Command_Length > sizeof( dbs->cmd ) ) ||
		( ( msddev->msddev_SubClass != MSD_SubClass_SCSI ) && ( dbs->dbs_Command_Length > 12 ) ) ||
		( ( dbs->dbs_Data_Length ) && ( ! dbs->dbs_Data ) ) || ( msddev->msddev_Detached ) )
	{
		goto bailout;
	}

	if ( ( msddev->msddev_RecoveryRequired ) && ( ! MSD_DoBulk_Recovery( usbbase, msddisk ) ) )
	{
		goto bailout;
	}

	// Zero ID is invalid.
	if ( ! ++msddev->msddev_Bulk_TagID )
	{
		msddev->msddev_Bulk_TagID++;
	}

	if ( MSD_Bulk_Cmd_Phase( usbbase, msddisk, dbs ) != PS_Okay )
	{
		goto recover;
	}

	if ( dbr )
	{
		dbr->CommandSent = TRUE;
	}

	if ( dbs->dbs_Data_Length )
	{
		stat = MSD_Bulk_Cmd_Data( usbbase, msddisk, dbs );

		if ( ( stat == PS_Error ) || ( msddev->msddev_Detached ) )
		{
			goto recover;
		}
	}

	if ( MSD_Bulk_Cmd_Status( usbbase, msddisk ) != PS_Okay )
	{
		goto recover;
	}

	status = msddisk->msddisk_CSW.csw_Status;
	residue = LE_SWAP32( msddisk->msddisk_CSW.csw_DataResidue );

	if ( ( LE_SWAP32( msddisk->msddisk_CSW.csw_Signature ) != MSDCSW_Signature ) ||
		( LE_SWAP32( msddisk->msddisk_CSW.csw_Tag ) != msddev->msddev_Bulk_TagID ) ||
		( status > MSDCSW_Status_CmdFailed ) || ( residue > dbs->dbs_Data_Length ) ||
		( dbs->dbs_Data_Actual > dbs->dbs_Data_Length ) )
	{
		MSDDEBUG( "MSD: Invalid CSW or data count for SCSI $%02lx (status %lu, residue %lu, USB actual %lu)\n",
			(U32) dbs->cmd[0], status, residue, dbs->dbs_Data_Actual );

		goto recover;
	}

	// OUT residue reports bytes processed, which can be fewer than bytes sent.
	// IN can include padding or a bridge that reports zero residue on a short read.
	actual = MIN( dbs->dbs_Data_Length - residue, dbs->dbs_Data_Actual );

	if ( dbr )
	{
		dbr->csw_DataResidue = residue;
		dbr->csw_DataActual = actual;
		dbr->csw_Status = status;
		dbr->csw_Valid = TRUE;
		dbr->csw_SigOK = TRUE;
		dbr->csw_TagOK = TRUE;
	}

	retval = ( status == MSDCSW_Status_CmdPassed ) &&
		( ( allow_short ) || ( actual == dbs->dbs_Data_Length ) );

	goto bailout;

recover:
	// Reset for the next command. A command without a CSW may have changed media.
	if ( ! msddev->msddev_Detached )
	{
		msddev->msddev_RecoveryRequired = TRUE;

		MSDDEBUG( "MSD: BOT recovery for SCSI $%02lx\n", (U32) dbs->cmd[0] );

		MSD_DoBulk_Recovery( usbbase, msddisk );
	}

bailout:
	TASK_NAME_LEAVE();

	return ( retval );
}

// --
