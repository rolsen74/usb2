
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

SEC_CODE void
MSD_Disk_NotifyChange( struct USBBase *usbbase, struct MSDDisk *msddisk )
{
	struct IOStdReq *ioreq;

	SEMAPHORE_OBTAIN( & msddisk->msddisk_MSDUnit_Semaphore );

	for( ioreq = Header_Head( & msddisk->msddisk_MSDUnit_ChangeList ); ioreq; ioreq = Node_Next( ioreq ) )
	{
		if ( ioreq->io_Data )
		{
			usbbase->usb_IExec->Cause( (PTR) ioreq->io_Data );
		}
	}

	SEMAPHORE_RELEASE( & msddisk->msddisk_MSDUnit_Semaphore );
}

// --
