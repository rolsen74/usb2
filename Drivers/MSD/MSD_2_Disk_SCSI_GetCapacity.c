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

	TASK_NAME_ENTER( "MSD_SCSI_GetCapacity" );

	retval = FALSE;

	if ( ( ! blocks ) || ( ! block_size ) )
	{
		goto bailout;
	}

	if ( ! MSD_SCSI_ReadCapacity10( usbbase, msddisk, NULL, capdata10, sizeof( capdata10 ) ) )
	{
		goto bailout;
	}

	last = BE_SWAP32( capdata10[0] );
	size = BE_SWAP32( capdata10[1] );

	if ( last == 0xffffffffULL )
	{
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
		goto bailout;
	}

	*blocks = last + 1;
	*block_size = size;
	retval = TRUE;

bailout:

	TASK_NAME_LEAVE();

	return ( retval );
}

// --
