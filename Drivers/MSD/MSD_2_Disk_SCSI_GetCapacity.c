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

SEC_CODE S32 MSD_SCSI_GetCapacity( struct USBBase *usbbase, struct MSDDisk *msddisk, U64 *blocks, U32 *block_size )
{
U32 capdata10[2];
U8 capdata16[32];
U64 last;
U32 size;
U32 cnt;
S32 retval;
#ifdef MSD_DEBUG
U32 command_length;
U32 invalid_geometry;
#endif

	TASK_NAME_ENTER( "MSD_SCSI_GetCapacity" );

	retval = FALSE;
#ifdef MSD_DEBUG
	command_length = 0;
	invalid_geometry = FALSE;
#endif

	if ( ( ! blocks ) || ( ! block_size ) )
	{
		goto bailout;
	}

#ifdef MSD_DEBUG
	command_length = 10;

	if ( ( MSD_TRACE_OPTICAL( msddisk ) ) && ( ! msddisk->msddisk_CapacityTrace.Seen ) )
	{
		MSDDEBUG( "MSD: interface %lu LUN %lu starting READ CAPACITY(10)\n",
			msddisk->msddisk_MSDDev->msddev_InterfaceNr, msddisk->msddisk_Lun );
	}
#endif

	if ( ! MSD_SCSI_ReadCapacity10( usbbase, msddisk, NULL, capdata10, sizeof( capdata10 ) ) )
	{
		goto bailout;
	}

	last = BE_SWAP32( capdata10[0] );
	size = BE_SWAP32( capdata10[1] );

	if ( last == 0xffffffffULL )
	{
#ifdef MSD_DEBUG
		command_length = 16;
#endif
		if ( ! MSD_SCSI_ReadCapacity16( usbbase, msddisk, capdata16, sizeof( capdata16 ) ) )
		{
			goto bailout;
		}

		last = 0;

		for( cnt=0 ; cnt<8 ; cnt++ )
		{
			last = ( last << 8 ) | capdata16[cnt];
		}

		size = ( (U32) capdata16[8] << 24 )
			 | ( (U32) capdata16[9] << 16 )
			 | ( (U32) capdata16[10] << 8 )
			 | (U32) capdata16[11];
	}

	if ( ( last == ~(U64) 0 ) || ( ! size ) || ( size & ( size - 1 ) ) )
	{
		USBERROR( "MSD_SCSI_GetCapacity : Invalid capacity or block size %lu", size );
#ifdef MSD_DEBUG
		invalid_geometry = TRUE;
#endif
		goto bailout;
	}

	*blocks = last + 1;
	*block_size = size;
	retval = TRUE;

bailout:

#ifdef MSD_DEBUG
	if ( ( command_length ) && ( MSD_TRACE_OPTICAL( msddisk ) ) &&
		( MSD_TraceDue( &msddisk->msddisk_CapacityTrace,
			( command_length << 2 ) | ( invalid_geometry << 1 ) | retval ) ) )
	{
		if ( retval )
		{
			MSDDEBUG( "MSD: interface %lu LUN %lu READ CAPACITY(%lu), $%08lX%08lX blocks of %lu bytes\n",
				msddisk->msddisk_MSDDev->msddev_InterfaceNr, msddisk->msddisk_Lun,
				command_length, (U32) ( *blocks >> 32 ), (U32) *blocks, *block_size );
		}
		else
		{
			MSDDEBUG( "MSD: interface %lu LUN %lu READ CAPACITY(%lu) %s\n",
				msddisk->msddisk_MSDDev->msddev_InterfaceNr, msddisk->msddisk_Lun,
				command_length, ( invalid_geometry ) ? "returned invalid geometry" : "failed" );
		}
	}
#endif

	TASK_NAME_LEAVE();

	return ( retval );
}

// --
