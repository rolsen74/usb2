
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
#if 1

SEC_CODE void
MSD_Cmd_0015_TD_RemChangeInt( struct USBBase *usbbase, struct IOStdReq *ioreq )
{
	struct MSDDisk *msddisk;
	struct IOStdReq *registered;

	TASK_NAME_ENTER( "MSD : _cmd_0015_TD_RemChangeInt" );
	USBERROR( "MSD : _cmd_0015_TD_RemChangeInt" );

	msddisk = (PTR) ioreq->io_Unit;
	ioreq->io_Actual = 0;

	if ( ! msddisk )
	{
		ioreq->io_Error = TDERR_BadUnitNum;

		TASK_NAME_LEAVE();

		return;
	}

	SEMAPHORE_OBTAIN( & msddisk->msddisk_MSDUnit_Semaphore );

	for( registered = Header_Head( & msddisk->msddisk_MSDUnit_ChangeList );
		( registered ) && ( registered != ioreq ); registered = Node_Next( registered ) )
	{
	}

	if ( ! registered )
	{
		USBERROR( "MSD : Unknwon IOReq %p", ioreq );

		ioreq->io_Error = TDERR_NotSpecified;
	}
	else
	{
		NODE_REMNODE( & msddisk->msddisk_MSDUnit_ChangeList, registered );

		ioreq->io_Error = 0;
	}

	SEMAPHORE_RELEASE( & msddisk->msddisk_MSDUnit_Semaphore );

	TASK_NAME_LEAVE();
}

#endif
// --
