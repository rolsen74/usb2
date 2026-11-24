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

SEC_CODE S32 MSD_SCSI_SynchronizeCache( struct USBBase *usbbase, struct MSDDisk *msddisk, struct DoBulkResult *result )
{
struct DoBulkStruct dbs;
S32 retval;

	TASK_NAME_ENTER( "MSD_SCSI_SynchronizeCache" );

	MEM_SET( & dbs, 0, sizeof( dbs ) );
	dbs.dbs_Result = result;
	dbs.dbs_Command_Length = 10;
	dbs.cmd[0] = 0x35;

	retval = MSD_Disk_DoBulk( usbbase, msddisk, & dbs );

	TASK_NAME_LEAVE();

	return ( retval );
}

// --
