
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

SEC_CODE S32
MSD_Cmd_0014_TD_AddChangeInt( struct USBBase *usbbase, struct IOStdReq *ioreq )
{
	struct MSDDisk *msddisk;
	struct IOStdReq *registered;

	TASK_NAME_ENTER( "MSD : _cmd_0014_TD_AddChangeInt" );
	USBERROR( "MSD : _cmd_0014_TD_AddChangeInt" );

	msddisk = (PTR) ioreq->io_Unit;

	if ( ! msddisk )
	{
		ioreq->io_Error = TDERR_BadUnitNum;

		TASK_NAME_LEAVE();

		return ( TRUE );
	}

	SEMAPHORE_OBTAIN( & msddisk->msddisk_MSDUnit_Semaphore );

	for( registered = Header_Head( & msddisk->msddisk_MSDUnit_ChangeList );
		( registered ) && ( registered != ioreq ); registered = Node_Next( registered ) )
	{
	}

	if ( ( ! registered ) && ( msddisk->msddisk_Detached ) )
	{
		SEMAPHORE_RELEASE( & msddisk->msddisk_MSDUnit_Semaphore );

		ioreq->io_Error = TDERR_DiskChanged;

		TASK_NAME_LEAVE();

		return ( TRUE );
	}

	if ( ! registered )
	{
		NODE_ADDTAIL( & msddisk->msddisk_MSDUnit_ChangeList, ioreq );
	}

	SEMAPHORE_RELEASE( & msddisk->msddisk_MSDUnit_Semaphore );

	TASK_NAME_LEAVE();

	// The registration stays pending until TD_REMCHANGEINT.
	return ( FALSE );
}

#endif
// --
