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

SEC_CODE S32 MSD_SCSI_GetWriteProtect( struct USBBase *usbbase, struct MSDDisk *msddisk, U32 *protected )
{
struct DoBulkResult result;
struct DoBulkStruct dbs;
U8 data[8];
S32 retval;
S32 error;
S32 ufi;

	TASK_NAME_ENTER( "MSD_SCSI_GetWriteProtect" );

	retval = FALSE;

	if ( ! protected )
	{
		goto bailout;
	}

	ufi = ( msddisk->msddisk_MSDDev->msddev_SubClass == MSD_SubClass_UFI );

	if ( ! ufi )
	{
		MEM_SET( & dbs, 0, sizeof( dbs ) );
		MEM_SET( data, 0, sizeof( data ) );
		dbs.dbs_Flags = DOBULKF_DIR_IN;
		dbs.dbs_Result = & result;
		dbs.dbs_Command_Length = 6;
		dbs.dbs_Data = data;
		dbs.dbs_Data_Length = 4;
		dbs.cmd[0] = 0x1a;
		dbs.cmd[1] = 0x08;
		dbs.cmd[2] = 0x3f;
		dbs.cmd[4] = 4;

		if ( ( MSD_Disk_DoBulk( usbbase, msddisk, & dbs ) ) && ( result.csw_DataActual == 4 ) && ( data[0] >= 3 ) )
		{
			*protected = ( data[2] & 0x80 ) ? TRUE : FALSE;
			retval = TRUE;
			goto bailout;
		}

		error = MSD_Disk_CommandError( usbbase, msddisk, &result );

		if ( error == TDERR_DiskChanged )
		{
			goto bailout;
		}

		if ( error == TDERR_WriteProt )
		{
			*protected = TRUE;
			retval = TRUE;
			goto bailout;
		}
	}

	MEM_SET( & dbs, 0, sizeof( dbs ) );
	MEM_SET( data, 0, sizeof( data ) );
	dbs.dbs_Flags = DOBULKF_DIR_IN;
	dbs.dbs_Result = & result;
	dbs.dbs_Command_Length = 10;
	dbs.dbs_Data = data;
	dbs.dbs_Data_Length = 8;
	dbs.cmd[0] = 0x5a;
	dbs.cmd[1] = ( ufi ) ? 0 : 0x08;
	dbs.cmd[2] = 0x3f;
	dbs.cmd[8] = 8;

	if ( ( MSD_Disk_DoBulk( usbbase, msddisk, & dbs ) ) && ( result.csw_DataActual == 8 ) && ( ( (U32) data[0] << 8 | data[1] ) >= 6 ) )
	{
		*protected = ( data[3] & 0x80 ) ? TRUE : FALSE;
		retval = TRUE;
	}
	else if ( MSD_Disk_CommandError( usbbase, msddisk, &result ) == TDERR_WriteProt )
	{
		*protected = TRUE;
		retval = TRUE;
	}

bailout:

	TASK_NAME_LEAVE();

	return ( retval );
}

// --
