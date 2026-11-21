
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

SEC_CODE S32 MSD_Disk_DoBulk( struct USBBase *usbbase, struct MSDDisk *msddisk, struct DoBulkStruct *dbs )
{
struct DoBulkResult *dbr;
struct MSDDevice *msddev;
enum PhaseStat stat;
enum PhaseStat data_stat;
U32 host_Tag;
U32 csw_Tag;
U32 residue;
U32 actual;
S32 short_input;
S32 retval;
S32 tries;

	TASK_NAME_ENTER( "MSD_Disk_DoBulk" );

	msddev	= msddisk->msddisk_MSDDev;

//	USBERROR( "MSD : MSD_Disk_DoBulk : Lun %ld : CmdLen %ld : Mem %p : MemLen %ld : Flags $%04lx", 
//		msddisk->msddisk_Lun, 
//		dbs->dbs_Command_Length, 
//		dbs->dbs_Data, 
//		dbs->dbs_Data_Length,
//		dbs->dbs_Flags
//	);

	retval	= FALSE;
	dbr		= dbs->dbs_Result;
	short_input = ( dbs->dbs_Data ) && ( ( dbs->dbs_Flags & DOBULKF_DIR_MASK ) == DOBULKF_DIR_IN ) &&
		( dbs->dbs_Flags & DOBULKF_SHORT_OK );

	if ( dbs->dbs_Command_Length > 16 )
	{
		#if defined( DO_ERROR ) || defined( DO_INFO ) || defined( DO_DEBUG )
		USBERROR( "Limiting CmdLen to 16 bytes (Len %ld)", dbs->dbs_Command_Length );
		#endif
		dbs->dbs_Command_Length = 16;
	}

	for( tries=0 ; tries<3 ; tries++ )
	{
		data_stat = PS_Okay;
		dbs->dbs_Data_Actual = 0;

		// Reset result struct for this attempt
		if ( dbr )
		{
			dbr->csw_DataResidue = 0;
			dbr->csw_DataActual	= 0;
			dbr->csw_Valid		= FALSE;
			dbr->csw_Status		= 0;
			dbr->csw_SigOK		= 0;
			dbr->csw_TagOK		= 0;
		}

		if ( msddev->msddev_Detached )
		{
			break;
		}

		// Zero ID is invalid
		while( ++ msddev->msddev_Bulk_TagID == 0 );

		// --

//		MSDDEBUG( "__phase_command : Calling Function\n" );

		stat = MSD_Bulk_Cmd_Phase( usbbase, msddisk, dbs );

//		MSDDEBUG( "__phase_command : Stat %ld\n", stat );

		if ( stat != PS_Okay )
		{
			goto retry;
		}

		if ( ( dbs->dbs_Data ) && ( dbs->dbs_Data_Length ) )
		{
			data_stat = MSD_Bulk_Cmd_Data( usbbase, msddisk, dbs );
		}

		if ( msddev->msddev_Detached )
		{
			break;
		}

		stat = MSD_Bulk_Cmd_Status( usbbase, msddisk );

		if ( ( ( stat != PS_Okay ) && ( stat != PS_Stall ) ) || ( data_stat == PS_Error ) )
		{
			goto retry;
		}

		if ( LE_SWAP32( msddisk->msddisk_CSW.csw_Signature ) != MSDCSW_Signature )
		{
			MSDDEBUG( "MSD: invalid CSW signature $%08lx\n", msddisk->msddisk_CSW.csw_Signature );

			goto retry;
		}

		host_Tag = msddev->msddev_Bulk_TagID;

		csw_Tag = LE_SWAP32( msddisk->msddisk_CSW.csw_Tag );

		if ( csw_Tag != host_Tag )
		{
			MSDDEBUG( "MSD: CSW tag $%08lx, expected $%08lx\n", csw_Tag, host_Tag );

			goto retry;
		}

		if ( ( msddisk->msddisk_CSW.csw_Status != MSDCSW_Status_CmdPassed ) &&
			( msddisk->msddisk_CSW.csw_Status != MSDCSW_Status_CmdFailed ) )
		{
			MSDDEBUG( "MSD: invalid CSW status %lu for SCSI $%02lx\n",
				(U32) msddisk->msddisk_CSW.csw_Status, (U32) dbs->cmd[0] );

			goto retry;
		}

		residue = LE_SWAP32( msddisk->msddisk_CSW.csw_DataResidue );

		if ( ( residue > dbs->dbs_Data_Length ) || ( dbs->dbs_Data_Actual > dbs->dbs_Data_Length ) )
		{
			MSDDEBUG( "MSD: SCSI $%02lx, requested %lu, USB actual %lu, CSW residue %lu\n",
				(U32) dbs->cmd[0], dbs->dbs_Data_Length, dbs->dbs_Data_Actual, residue );

			goto retry;
		}

		actual = dbs->dbs_Data_Length - residue;

		if ( actual != dbs->dbs_Data_Actual )
		{
			if ( ( ! short_input ) || ( data_stat != PS_Okay ) )
			{
				MSDDEBUG( "MSD: SCSI $%02lx, USB actual %lu differs from CSW count %lu\n",
					(U32) dbs->cmd[0], dbs->dbs_Data_Actual, actual );

				goto retry;
			}

			// A short IN result must fit both the received bytes and the CSW's relevant data.
			actual = MIN( actual, dbs->dbs_Data_Actual );
			residue = dbs->dbs_Data_Length - actual;
		}

		retval = ( msddisk->msddisk_CSW.csw_Status == MSDCSW_Status_CmdPassed );

		if ( ( retval ) && ( ( data_stat != PS_Okay ) || ( ( ! short_input ) && ( residue ) ) ) )
		{
			retval = FALSE;

			goto retry;
		}

		if ( dbr )
		{
			dbr->csw_DataResidue = residue;
			dbr->csw_DataActual = actual;
			dbr->csw_Valid = TRUE;
			dbr->csw_Status = msddisk->msddisk_CSW.csw_Status;
			dbr->csw_SigOK = TRUE;
			dbr->csw_TagOK = TRUE;
		}

		break;

retry:
		if ( msddev->msddev_Detached )
		{
			break;
		}

		MSDDEBUG( "MSD: BOT recovery for SCSI $%02lx, attempt %ld\n", (U32) dbs->cmd[0], tries + 1 );

		if ( ! MSD_DoBulk_Recovery( usbbase, msddisk ) )
		{
			break;
		}
	}

	TASK_NAME_LEAVE();

	return ( retval );
}

// --
