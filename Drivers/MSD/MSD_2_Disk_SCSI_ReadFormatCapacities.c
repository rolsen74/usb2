
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

SEC_CODE U32
MSD_SCSI_UnformattedMedia( struct USBBase *usbbase, struct MSDDisk *msddisk )
{
	struct DoBulkResult result;
	struct DoBulkStruct dbs;

	U8 data[12];

	MEM_SET( &dbs, 0, sizeof( dbs ) );

	dbs.dbs_Result = &result;
	dbs.dbs_Flags = DOBULKF_DIR_IN;
	dbs.dbs_Command_Length = 10;
	dbs.dbs_Data = data;
	dbs.dbs_Data_Length = sizeof( data );
	dbs.cmd[0] = 0x23;
	dbs.cmd[8] = sizeof( data );

	if ( ! MSD_Disk_DoBulk( usbbase, msddisk, &dbs ) )
	{
		MSD_Disk_CommandError( usbbase, msddisk, &result );

		return ( FALSE );
	}

	// The first descriptor distinguishes unformatted media from an empty drive.
	return ( ( data[3] >= 8 ) && ( ( data[8] & 3 ) == 1 ) );
}

// --
