
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
MSD_Cmd_A007_NSCmd_RemStatCallBack( struct USBBase *usbbase, struct IOStdReq *ioreq )
{
	struct ExecIFace *IExec;
	struct MSDDisk *msddisk;
	struct IOStdReq *registered;
	struct MsgPort *reply;
	struct Node *node;

	msddisk = (PTR) ioreq->io_Unit;
	IExec = usbbase->usb_IExec;

	if ( ! msddisk )
	{
		ioreq->io_Actual = 0;
		ioreq->io_Error = TDERR_BadUnitNum;
		ioreq->io_Flags &= ~IOF_QUICK;

		MSGPORT_REPLYMSG( ioreq );

		return;
	}

	IExec->ObtainSemaphore( &msddisk->msddisk_StatSemaphore );

	registered = MSD_Disk_RemoveStatCallback( usbbase, msddisk, ioreq, NULL );

	if ( ! registered )
	{
		reply = ioreq->io_Message.mn_ReplyPort;
		node = NULL;

		IExec->Disable();

		if ( reply )
		{
			for ( node = IExec->GetHead( &reply->mp_MsgList ); node; node = IExec->GetSucc( node ) )
			{
				if ( node == &ioreq->io_Message.mn_Node )
				{
					break;
				}
			}
		}

		IExec->Enable();

		// Abort or detach already completed this ADD request.
		if ( node )
		{
			IExec->ReleaseSemaphore( &msddisk->msddisk_StatSemaphore );

			return;
		}

		if ( ioreq->io_Data )
		{
			registered = MSD_Disk_RemoveStatCallback( usbbase, msddisk, NULL, ioreq->io_Data );
		}
	}

	if ( ( registered ) && ( registered != ioreq ) )
	{
		registered->io_Actual = 0;
		registered->io_Error = 0;

		MSGPORT_REPLYMSG( registered );
	}

	ioreq->io_Actual = 0;
	ioreq->io_Error = registered ? 0 : TDERR_NotSpecified;
	ioreq->io_Flags &= ~IOF_QUICK;

	MSGPORT_REPLYMSG( ioreq );

	IExec->ReleaseSemaphore( &msddisk->msddisk_StatSemaphore );
}

// --
