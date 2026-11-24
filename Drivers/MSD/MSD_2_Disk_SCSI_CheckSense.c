
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

SEC_CODE S32
MSD_Disk_ApplySense( struct USBBase *usbbase, struct MSDDisk *msddisk, const U8 *sense, U32 length )
{
	U8 format;
	U8 key;
	U8 asc;
#ifdef MSD_DEBUG
	U8 ascq;
#endif

	if ( ( ! sense ) || ( length < 4 ) )
	{
		return ( TDERR_NotSpecified );
	}

	format = sense[0] & 0x7f;

	if ( ( format == 0x72 ) || ( format == 0x73 ) )
	{
		key = sense[1] & 0x0f;
		asc = sense[2];
#ifdef MSD_DEBUG
		ascq = sense[3];
#endif
	}
	else if ( ( ( format == 0x70 ) || ( format == 0x71 ) ) && ( length >= 14 ) )
	{
		key = sense[2] & 0x0f;
		asc = sense[12];
#ifdef MSD_DEBUG
		ascq = sense[13];
#endif
	}
	else
	{
#ifdef MSD_DEBUG
		if ( ( MSD_TRACE_OPTICAL( msddisk ) ) &&
			( MSD_TraceDue( &msddisk->msddisk_SenseTrace, 0x20000000U | ( (U32) format << 8 ) | length ) ) )
		{
			MSDDEBUG( "MSD: interface %lu LUN %lu sense format $%02lX, %lu bytes, no complete key/ASC/ASCQ\n",
				msddisk->msddisk_MSDDev->msddev_InterfaceNr, msddisk->msddisk_Lun, (U32) format, length );
		}
#endif
		return ( TDERR_NotSpecified );
	}

#ifdef MSD_DEBUG
	if ( ( MSD_TRACE_OPTICAL( msddisk ) ) &&
		( MSD_TraceDue( &msddisk->msddisk_SenseTrace,
			0x80000000U | ( (U32) ( format & 3 ) << 28 ) | ( (U32) key << 24 ) | ( MIN( length, 255U ) << 16 ) |
			( (U32) asc << 8 ) | ascq ) ) )
	{
		MSDDEBUG( "MSD: interface %lu LUN %lu sense format $%02lX, %lu bytes, key $%02lX ASC/ASCQ $%02lX/$%02lX\n",
			msddisk->msddisk_MSDDev->msddev_InterfaceNr, msddisk->msddisk_Lun,
			(U32) format, length, (U32) key, (U32) asc, (U32) ascq );
	}
#endif

	if ( ( ( key == 0x02 ) && ( asc == 0x3a ) ) || ( ( key == 0x06 ) && ( asc == 0x28 ) ) )
	{
		if ( ( msddisk->msddisk_Removable ) && ( msddisk->msddisk_MediaPresent ) )
		{
			msddisk->msddisk_MediaPresent = FALSE;
			msddisk->msddisk_TotalBlocks = 0;
			msddisk->msddisk_Block_Size = 0;
			msddisk->msddisk_Block_Mask = 0;
			msddisk->msddisk_Block_Shift = 0;
			msddisk->msddisk_DiskChangeCount++;

			MSD_Disk_CancelWrites( usbbase, msddisk );

			MSD_Disk_NotifyChange( usbbase, msddisk );
		}

		return ( TDERR_DiskChanged );
	}

	if ( key == 0x07 )
	{
		return ( TDERR_WriteProt );
	}

	if ( ( key == 0x03 ) && ( asc == 0x11 ) )
	{
		return ( TDERR_BadSecSum );
	}

	if ( ( key == 0x05 ) && ( asc == 0x21 ) )
	{
		return ( IOERR_BADADDRESS );
	}

	return ( TDERR_NotSpecified );
}

// --

SEC_CODE S32
MSD_Disk_CommandError( struct USBBase *usbbase, struct MSDDisk *msddisk, const struct DoBulkResult *result )
{
	struct DoBulkResult sense_result;

	U8 sense[18];

	if ( ( result->csw_Valid ) && ( result->csw_Status == MSDCSW_Status_CmdFailed ) &&
		( ! msddisk->msddisk_MSDDev->msddev_Detached ) )
	{
		if ( MSD_SCSI_RequestSense( usbbase, msddisk, & sense_result, sense, sizeof( sense ) ) )
		{
			return ( MSD_Disk_ApplySense( usbbase, msddisk, sense, sense_result.csw_DataActual ) );
		}
	}

	return ( msddisk->msddisk_MSDDev->msddev_Detached ? TDERR_DiskChanged : TDERR_NotSpecified );
}

// --
