
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
MSD_DoBulk_Recovery( struct USBBase *usbbase, struct MSDDisk *msddisk )
{
	struct USB2_IORequest *ioreq;
	struct USB2_SetupData *sd;
	struct MSDDevice *msddev;

	S32 result;

	TASK_NAME_ENTER( "MSD_DoBulk_Recovery" );

	msddev = msddisk->msddisk_MSDDev;
	result = FALSE;

	if ( ( msddev->msddev_Detached ) || ( msddisk->msddisk_InRecovery ) )
	{
		goto bailout;
	}

	msddisk->msddisk_InRecovery = TRUE;
	sd = msddev->msddev_Res_Control->SetupData;
	sd->RequestType = REQTYPE_Write | REQTYPE_Class | REQTYPE_Interface;
	sd->RequestCode = REQCODE_MassStorage_Reset;
	sd->Value = LE_SWAP16( 0 );
	sd->Index = LE_SWAP16( msddev->msddev_InterfaceNr );
	sd->Length = LE_SWAP16( 0 );

	ioreq = msddev->msddev_Res_Control->IORequests[0];
	ioreq->io_Command = CMD_WRITE;
	ioreq->io_Data = NULL;
	ioreq->io_Length = 0;
	ioreq->io_SetupData = sd;
	ioreq->io_SetupLength = sizeof( *sd );

	IO_DO( ioreq );

	if ( ioreq->io_Error )
	{
		MSDDEBUG( "MSD: Bulk-Only Reset failed, USB error %ld\n", (S32) ioreq->io_Error );

		if ( ioreq->io_Error == USB2Err_Device_Detached )
		{
			MSD_Disk_Detach( usbbase, msddisk );
		}
	}
	else
	{
		MISC_MICRODELAY( 10 * 1000 );

		if ( ( ! msddev->msddev_Detached ) && ( ENDPOINTRES_DESTALL( msddev->msddev_Res_Bulk_In ) ) &&
			( ! msddev->msddev_Detached ) && ( ENDPOINTRES_DESTALL( msddev->msddev_Res_Bulk_Out ) ) )
		{
			MISC_MICRODELAY( 5 * 1000 );

			result = ! msddev->msddev_Detached;
		}
	}

	msddisk->msddisk_InRecovery = FALSE;
	msddev->msddev_RecoveryRequired = ! result;

bailout:

	TASK_NAME_LEAVE();

	return ( result );
}

// --
