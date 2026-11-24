
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

#define MSD_CSW_TIMEOUT_US		500000
#define MSD_CDROM_READY_TIMEOUT_US	5000000

// --

SEC_CODE enum PhaseStat MSD_Bulk_Cmd_Status( struct USBBase *usbbase, struct MSDDisk *msddisk )
{
struct USB2_EPResource *epr;
struct USB2_IORequest *ioreq;
struct MSDDevice *msddev;
//struct MSDDisk *msddisk;
enum PhaseStat stat;
U32 timeout;
S32 tries;

	TASK_NAME_ENTER( "MSD_Bulk_Cmd_Status" );

	msddev	= msddisk->msddisk_MSDDev;
//	msddisk	= msddisk->msdunit_MSDDisk;

//	USBERROR( "MSD : MSD_Disk_DoBulk : MSD_Bulk_Cmd_Status : Lun %ld : Cmd %p : CmdLen %ld : Mem %p : MemLen %ld : Flags $%04lx", msddisk->msddisk_Lun, cmd, cmdlen, mem, memlen, Flags );

	stat	= PS_Error;
	epr		= msddev->msddev_Res_Bulk_In;
	ioreq	= epr->IORequests[0];
	timeout	= MSD_CSW_TIMEOUT_US;

	// Some optical drives finish spinning up before replying to TEST UNIT READY.
	switch ( msddisk->msddisk_CBW.cbw_CB[0] )
	{
		case 0x1b: // START STOP UNIT
		case 0x35: // SYNCHRONIZE CACHE(10)
		case 0x91: // SYNCHRONIZE CACHE(16)
			timeout = 30 * 1000000;
			break;

		case 0x00: // TEST UNIT READY
			if ( msddisk->msddisk_DeviceType == DG_CDROM )
			{
				timeout = MSD_CDROM_READY_TIMEOUT_US;
			}
			break;
	}

	for( tries=0 ; tries<2 ; tries++ )
	{
		#if defined( DO_ERROR ) || defined( DO_INFO ) || defined( DO_DEBUG )
		MEM_SET( & msddisk->msddisk_CSW, 0xdc, sizeof( struct MSDCSW ));
		#endif

		ioreq->io_Command		= CMD_READ;
		ioreq->io_Data			= & msddisk->msddisk_CSW;
		ioreq->io_Length		= sizeof( struct MSDCSW );
		ioreq->io_SetupData		= NULL;
		ioreq->io_SetupLength	= 0;
		ioreq->io_TimeOut		= timeout;

		IO_DO( ioreq );

//		MSDDEBUG( "CSW  : io_Error = %ld : io_Actual %ld\n", ioreq->io_Error, ioreq->io_Actual );

		/**/ if ( ioreq->io_Error == USB2Err_NoError )
		{
			// I hard coded the Actual value too validate 'struct MSDCSW' size
			if ( ioreq->io_Actual == MSDCSW_LEN )
			{
				stat = PS_Okay;
			}
			else
			{
				// Short/long read without STALL → invalid CSW, fail fast.
				MSDDEBUG( "CSW : unexpected size %ld (wanted %ld)\n", ioreq->io_Actual, (S32) sizeof( struct MSDCSW ));
				stat = PS_Error;
			}
			break;
		}
		else if ( ioreq->io_Error == USB2Err_Device_Detached )
		{
			USBERROR( "MSD_Bulk_Cmd_Status : Detached" );
			MSD_Disk_Detach( usbbase, msddisk );
			stat = PS_Error;
			break;
		}
		else if ( ioreq->io_Error == USB2Err_Host_Stall )
		{
			MSDDEBUG( "MSD: LUN %lu CSW stalled on attempt %ld, %lu/%lu bytes\n",
				msddisk->msddisk_Lun, tries + 1, ioreq->io_Actual, (U32) MSDCSW_LEN );

			if ( ( ! tries ) && ( ENDPOINTRES_DESTALL( epr ) ) )
			{
				continue;
			}

			break;
		}
		else
		{
			// Any other transport error (timeout, CRC, etc.) → fail
			MSDDEBUG( "MSD: LUN %lu SCSI $%02lX CSW USB error %ld, %lu/%lu bytes, timeout %lu ms\n",
				msddisk->msddisk_Lun, (U32) msddisk->msddisk_CBW.cbw_CB[0], (S32) ioreq->io_Error,
				ioreq->io_Actual, (U32) MSDCSW_LEN, timeout / 1000 );

			stat = PS_Error;
			break;
		}
	}

	#if 0
	// -- Lets dump struct
	U32 __pos = 0;
	U8 *__data = (PTR) & msddisk->msddisk_CSW;
	MSDDEBUG( "\n\n### CSW Dump (Status) ###\n" );
	MSDDEBUG( "CSW: Signature: $%02lx %02lx %02lx %02lx\n", __data[__pos+0], __data[__pos+1], __data[__pos+2], __data[__pos+3] );
	__pos += 4;
	MSDDEBUG( "CSW: Tag      : $%02lx %02lx %02lx %02lx\n", __data[__pos+0], __data[__pos+1], __data[__pos+2], __data[__pos+3] );
	__pos += 4;
	MSDDEBUG( "CSW: DResidue : $%02lx %02lx %02lx %02lx\n", __data[__pos+0], __data[__pos+1], __data[__pos+2], __data[__pos+3] );
	__pos += 4;
	MSDDEBUG( "CSW: Status   : $%02lx\n", __data[__pos] );
	__pos += 1;
	MSDDEBUG( "__pos %ld\n", __pos );
	#endif

	TASK_NAME_LEAVE();
	return( stat );
}

// --
