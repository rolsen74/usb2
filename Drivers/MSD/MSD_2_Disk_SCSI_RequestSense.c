
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

SEC_CODE S32 MSD_SCSI_RequestSense(
	struct USBBase *usbbase,
	struct MSDDisk *msddisk,
	struct DoBulkResult *dbr,
	PTR mem,
	U32 reqlen )
{
struct DoBulkResult result;
struct DoBulkStruct dbs;
S32 retval;
	S32 completed;
U32 len;

	TASK_NAME_ENTER( "MSD_SCSI_RequestSense" );
	USBERROR( "MSD : MSD_SCSI_RequestSense" );

	retval = FALSE;

	if ( ( ! mem ) || ( reqlen < 4 ) )
	{
		goto bailout;
	}

	len = MIN( reqlen, 255 );

	// --
	MEM_SET( & dbs, 0, sizeof( dbs ));
	dbs.dbs_Flags			= DOBULKF_DIR_IN | DOBULKF_SHORT_OK;
	dbs.dbs_Result			= & result;
	dbs.dbs_Command_Length	= 6;
	dbs.dbs_Data			= mem;
	dbs.dbs_Data_Length		= len;
	dbs.cmd[ 0]	= 0x03;				// REQUEST SENSE;
//	dbs.cmd[ 1]	= 0;				// Desc bit & reserved
//	dbs.cmd[ 2]	= 0;				// Reserved
//	dbs.cmd[ 3]	= 0;				// Reserved
	dbs.cmd[ 4]	= ( len & 0xff );	// Length 4-255 bytes
//	dbs.cmd[ 5]	= 0;				// Control
//	dbs.cmd[ 6]	= 0;
//	dbs.cmd[ 7]	= 0;
//	dbs.cmd[ 8]	= 0;
//	dbs.cmd[ 9]	= 0;
//	dbs.cmd[10]	= 0;
//	dbs.cmd[11]	= 0;
//	dbs.cmd[12]	= 0;
//	dbs.cmd[13]	= 0;
//	dbs.cmd[14]	= 0;
//	dbs.cmd[15]	= 0;

	// --
	completed = MSD_Disk_DoBulk( usbbase, msddisk, &dbs );

#ifdef MSD_DEBUG
	if ( ( MSD_TRACE_OPTICAL( msddisk ) ) && ( ( ! completed ) || ( result.csw_DataActual < 4 ) ) &&
		( MSD_TraceDue( &msddisk->msddisk_SenseTrace,
			0x40000000U | ( (U32) result.CommandSent << 24 ) | ( (U32) result.csw_Valid << 16 ) |
			( (U32) result.csw_Status << 8 ) | result.csw_DataActual ) ) )
	{
		MSDDEBUG( "MSD: interface %lu LUN %lu REQUEST SENSE failed or short, %lu/%lu bytes, CBW sent %lu, CSW valid %lu status $%02lX\n",
			msddisk->msddisk_MSDDev->msddev_InterfaceNr, msddisk->msddisk_Lun,
			result.csw_DataActual, len, (U32) result.CommandSent, (U32) result.csw_Valid, (U32) result.csw_Status );
	}
#endif

	if ( ! completed )
	{
		USBERROR( "MSD_SCSI_RequestSense() failed" );
		goto bailout;
	}

	if ( result.csw_DataActual < 4 )
	{
		USBERROR( "MSD_SCSI_RequestSense() returned only %lu bytes", result.csw_DataActual );
		goto bailout;
	}

	if ( dbr )
	{
		*dbr = result;
	}

	#if 1
{
	U8 *sense = (U8 *)mem;

	if ( ( result.csw_DataActual >= 14 ) && ( ( sense[0] & 0x7F ) == 0x70 ) )
	{
		U8 senseKey UNUSED	= sense[2] & 0x0F;
		U8 asc UNUSED		= sense[12];
		U8 ascq UNUSED		= sense[13];

		USBDEBUG( "MSD : Sense Key: $%02lx  ASC: $%02lx  ASCQ: $%02lx", senseKey, asc, ascq );
	}
	else
	{
		USBDEBUG( "MSD : Sense Key: Failed" );
	}
}
	#endif

	retval = TRUE;

bailout:

	TASK_NAME_LEAVE();
	return( retval );
}

// --
