
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

SEC_CODE enum PhaseStat MSD_Bulk_Cmd_Data( struct USBBase *usbbase, struct MSDDisk *msddisk, struct DoBulkStruct *dbs )
{
struct USB2_EPResource *epr;
struct USB2_IORequest *ioreq;
struct MSDDevice *msddev;
//struct MSDDisk *msddisk;
enum PhaseStat stat;
U32 IsRead;

	TASK_NAME_ENTER( "MSD_Bulk_Cmd_Data" );

	msddev	= msddisk->msddisk_MSDDev;
//	msddisk	= msddisk->msdunit_MSDDisk;

//	USBERROR( "MSD : MSD_Disk_DoBulk : MSD_Bulk_Cmd_Data : Lun %ld : Cmd %p : CmdLen %ld : Mem %p : MemLen %ld : Flags $%04lx", msddisk->msddisk_Lun, cmd, cmdlen, mem, memlen, Flags );

	IsRead	= (( dbs->dbs_Flags & DOBULKF_DIR_MASK ) == DOBULKF_DIR_IN );

	stat	= PS_Error;
	epr		= ( IsRead ) ? msddev->msddev_Res_Bulk_In :  msddev->msddev_Res_Bulk_Out;
	ioreq	= epr->IORequests[0];

//	MSDDEBUG( "Data Block is : %s : Mem %p : MemLen %ld\n", ( IsRead ) ? "Read" : "Write", mem, memlen );

	ioreq->io_Command		= ( IsRead ) ? CMD_READ : CMD_WRITE;
	ioreq->io_Data			= dbs->dbs_Data;
	ioreq->io_Length		= dbs->dbs_Data_Length;
	ioreq->io_SetupData		= NULL;
	ioreq->io_SetupLength	= 0;
	ioreq->io_TimeOut		= 5 * 1000000;		// 5 sec

	IO_DO( ioreq );
	dbs->dbs_Data_Actual = ioreq->io_Actual;

//	MSDDEBUG( "Data : io_Error = %ld : io_Actual %ld\n", ioreq->io_Error, ioreq->io_Actual );

	/**/ if ( ioreq->io_Error == USB2Err_NoError )
	{
		// IN short read is normal; OUT short write is suspicious.
		if (( ! IsRead ) && ( ioreq->io_Actual != dbs->dbs_Data_Length )) 
		{
			MSDDEBUG( "Data : short OUT (actual %lu of %lu)\n", ioreq->io_Actual, dbs->dbs_Data_Length );

			// Still return OK; CSW will decide if this is acceptable.
		}

		stat = PS_Okay;
	}
	else if ( ioreq->io_Error == USB2Err_Device_Detached )
	{
		USBERROR( "MSD_Bulk_Cmd_Data : Detached" );
		MSD_Disk_Detach( usbbase, msddisk );
		stat = PS_Error;
	}
	else if ( ioreq->io_Error == USB2Err_Host_Stall )
	{
		MSDDEBUG( "MSD: LUN %lu SCSI $%02lX data stalled, %lu/%lu bytes\n",
			msddisk->msddisk_Lun, (U32) dbs->cmd[0], ioreq->io_Actual, dbs->dbs_Data_Length );

		if ( ENDPOINTRES_DESTALL( epr ) )
		{
			stat = PS_Stall;
		}
	}
	else
	{
		// Any other transport error (timeout, CRC, etc.): let caller recover
		MSDDEBUG( "MSD: LUN %lu SCSI $%02lX data USB error %ld, %lu/%lu bytes\n",
			msddisk->msddisk_Lun, (U32) dbs->cmd[0], (S32) ioreq->io_Error,
			ioreq->io_Actual, dbs->dbs_Data_Length );

		stat = PS_Error;
	}

	TASK_NAME_LEAVE();
	return( stat );
}

// --
