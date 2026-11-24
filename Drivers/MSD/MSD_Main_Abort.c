
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

SEC_CODE static struct IOStdReq *
ExtractQueued( struct USBBase *usbbase, struct MSDDisk *msddisk, struct IOStdReq *wanted, struct List *removed )
{
	struct USB2_MsgPort *port;
	struct IOStdReq *ioreq;
	struct ExecIFace *IExec;
	struct List *queue;
	struct Node *node;
	struct Node *next;

	IExec = usbbase->usb_IExec;
	ioreq = NULL;

	SEMAPHORE_OBTAIN( &msddisk->msddisk_MSDBase->msdbase_MSDDisk_Semaphore );

	port = msddisk->msddisk_MsgPort_Begin;

	if ( port )
	{
		queue = &port->ump_MsgPort.mp_MsgList;

		for ( node = IExec->GetHead( queue ); node; node = next )
		{
			next = IExec->GetSucc( node );
			ioreq = (PTR) node;

			if ( ( wanted == ioreq ) ||
				( ( ! wanted ) && ( ioreq->io_Unit == (PTR) msddisk ) &&
				  ( ( ioreq->io_Command == CMD_WRITE ) || ( ioreq->io_Command == TD_WRITE64 ) ||
				    ( ioreq->io_Command == NSCMD_TD_WRITE64 ) || ( ioreq->io_Command == TD_FORMAT ) ||
				    ( ioreq->io_Command == TD_FORMAT64 ) || ( ioreq->io_Command == NSCMD_TD_FORMAT64 ) ||
				    ( ioreq->io_Command == CMD_UPDATE ) ) ) )
			{
				IExec->Remove( node );

				if ( wanted )
				{
					break;
				}

				IExec->AddTail( removed, node );
			}
		}

		if ( ! node )
		{
			ioreq = NULL;
		}
	}

	SEMAPHORE_RELEASE( &msddisk->msddisk_MSDBase->msdbase_MSDDisk_Semaphore );

	return ( ioreq );
}

// --

SEC_CODE struct IOStdReq *
MSD_Queue_Remove( struct USBBase *usbbase, struct MSDDisk *msddisk, struct IOStdReq *wanted )
{
	return ( ExtractQueued( usbbase, msddisk, wanted, NULL ) );
}

// --

SEC_CODE void
MSD_Disk_CancelWrites( struct USBBase *usbbase, struct MSDDisk *msddisk )
{
	struct IOStdReq *ioreq;
	struct List removed;

	usbbase->usb_IExec->NewList( &removed );

	// Only the worker detects media changes. It owns the active request.
	ExtractQueued( usbbase, msddisk, NULL, &removed );

	while ( ( ioreq = (PTR) usbbase->usb_IExec->RemHead( &removed ) ) )
	{
		ioreq->io_Actual = 0;
		ioreq->io_Error = TDERR_DiskChanged;

		MSGPORT_REPLYMSG( ioreq );
	}
}

// --
