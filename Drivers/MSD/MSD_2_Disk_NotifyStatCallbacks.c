
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
MSD_Disk_NotifyStatCallbacks( struct USBBase *usbbase, struct IOStdReq *ioreq )
{
	struct MSDDisk *msddisk;
	struct MSDStatCallback *callback;
	struct StatData data;

	U32 flags;

	if ( ( ioreq->io_Error ) || ( ! ioreq->io_Actual ) || ( ! ioreq->io_Unit ) )
	{
		return;
	}

	switch( ioreq->io_Command )
	{
		case CMD_READ:
		case TD_READ64:
		case NSCMD_TD_READ64:
			flags = SD_READ;
			break;

		case CMD_WRITE:
		case TD_WRITE64:
		case NSCMD_TD_WRITE64:
		case TD_FORMAT:
		case TD_FORMAT64:
		case NSCMD_TD_FORMAT64:
			flags = SD_WRITE;
			break;

		default:
			return;
	}

	msddisk = (PTR) ioreq->io_Unit;

	usbbase->usb_IExec->ObtainSemaphore( & msddisk->msddisk_StatSemaphore );

	msddisk->msddisk_StatNext = Header_Head( & msddisk->msddisk_MSDUnit_StatList );

	while(( callback = msddisk->msddisk_StatNext ))
	{
		// Removal can run from inside a hook. Advance before calling it.
		msddisk->msddisk_StatNext = Node_Next( callback );
		data.Size = ioreq->io_Actual;
		data.Flags = flags;

		usbbase->usb_IUtility->CallHookPkt( callback->Hook, NULL, & data );
	}

	usbbase->usb_IExec->ReleaseSemaphore( & msddisk->msddisk_StatSemaphore );
}

// --
