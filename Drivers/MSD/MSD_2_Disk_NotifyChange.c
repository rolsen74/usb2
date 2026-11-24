
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

#ifdef MSD_DEBUG
	U32 notified;

	notified = 0;
#endif

	SEMAPHORE_OBTAIN( & msddisk->msddisk_MSDUnit_Semaphore );

	for( ioreq = Header_Head( & msddisk->msddisk_MSDUnit_ChangeList ); ioreq; ioreq = Node_Next( ioreq ) )
	{
		if ( ioreq->io_Data )
		{
			usbbase->usb_IExec->Cause( (PTR) ioreq->io_Data );
#ifdef MSD_DEBUG
			notified++;
#endif
		}
	}

	SEMAPHORE_RELEASE( & msddisk->msddisk_MSDUnit_Semaphore );

#ifdef MSD_DEBUG
	if ( MSD_TRACE_OPTICAL( msddisk ) )
	{
		MSDDEBUG( "MSD: interface %lu LUN %lu media %lu, change count %lu, disk-change listeners signalled %lu\n",
			msddisk->msddisk_MSDDev->msddev_InterfaceNr, msddisk->msddisk_Lun,
			(U32) msddisk->msddisk_MediaPresent, msddisk->msddisk_DiskChangeCount, notified );
	}
#endif
}

// --
