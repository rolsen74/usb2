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

SEC_CODE S32 MSD_SCSI_ReadCapacity16( struct USBBase *usbbase, struct MSDDisk *msddisk, PTR mem, U32 len )
{
struct DoBulkResult result;
struct DoBulkStruct dbs;
S32 retval;

	TASK_NAME_ENTER( "MSD_SCSI_ReadCapacity16" );

	retval = FALSE;

	if ( ( ! mem ) || ( len < 12 ) )
	{
		goto bailout;
	}

	MEM_SET( & dbs, 0, sizeof( dbs ) );
	dbs.dbs_Flags = DOBULKF_DIR_IN;
	dbs.dbs_Result = & result;
	dbs.dbs_Command_Length = 16;
	dbs.dbs_Data = mem;
	dbs.dbs_Data_Length = len;
	dbs.cmd[0] = 0x9e;
	dbs.cmd[1] = 0x10;
	dbs.cmd[10] = ( len >> 24 ) & 0xff;
	dbs.cmd[11] = ( len >> 16 ) & 0xff;
	dbs.cmd[12] = ( len >> 8 ) & 0xff;
	dbs.cmd[13] = len & 0xff;

	if ( ! MSD_Disk_DoBulk( usbbase, msddisk, & dbs ) )
	{
		MSD_Disk_CommandError( usbbase, msddisk, &result );

		goto bailout;
	}

	if ( result.csw_DataActual != len )
	{
		goto bailout;
	}

	retval = TRUE;

bailout:

	TASK_NAME_LEAVE();

	return ( retval );
}

// --
