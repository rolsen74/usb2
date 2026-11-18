
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

SEC_CODE struct IOStdReq *
MSD_Disk_RemoveStatCallback( struct USBBase *usbbase, struct MSDDisk *msddisk, struct IOStdReq *ioreq, struct Hook *hook )
{
	struct MSDStatCallback *callback;
	struct IOStdReq *result;

	result = NULL;

	usbbase->usb_IExec->ObtainSemaphore( & msddisk->msddisk_StatSemaphore );

	for( callback = Header_Head( & msddisk->msddisk_MSDUnit_StatList ); callback; callback = Node_Next( callback ) )
	{
		if ( ( callback->Request == ioreq ) || ( hook && callback->Hook == hook ) )
		{
			if ( msddisk->msddisk_StatNext == callback )
			{
				msddisk->msddisk_StatNext = Node_Next( callback );
			}

			result = callback->Request;

			NODE_REMNODE( & msddisk->msddisk_MSDUnit_StatList, callback );

			MEM_FREEVEC( callback );

			break;
		}
	}

	usbbase->usb_IExec->ReleaseSemaphore( & msddisk->msddisk_StatSemaphore );

	return ( result );
}

// --
