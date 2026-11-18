
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

S32
_usb_Read_Events( struct USB_Struct *us, struct USB2_IORequest *io )
{
	struct CameraInfoNode *camera;

	const U8 *data;
	U32 remaining;
	U32 length;
	U32 count;
	U32 storage;
	U16 type;
	U16 code;
	S32 changed;

	camera = & us->us_CameraNode;
	data = io->io_Data;
	remaining = io->io_Actual;
	changed = FALSE;

	if ( io->io_Error == USB2Err_Device_Detached )
	{
		us->us_Detached = TRUE;
	}

	if ( ( io->io_Error ) || ( remaining > io->io_Length ) )
	{
		MYINFO( "PTP-USB : Event read failed (%ld)", (S32) io->io_Error );

		return ( FALSE );
	}

	while( remaining )
	{
		if ( us->us_EventUsed < sizeof( length ) )
		{
			count = MIN( remaining, sizeof( length ) - us->us_EventUsed );

			memcpy( us->us_EventData + us->us_EventUsed, data, count );

			us->us_EventUsed += count;
			data += count;
			remaining -= count;

			if ( us->us_EventUsed < sizeof( length ) )
			{
				break;
			}
		}

		memcpy( & length, us->us_EventData, sizeof( length ) );

		length = LE_SWAP32( length );

		if ( ( length < 12 ) || ( length > sizeof( us->us_EventData ) ) || ( length & 3 ) )
		{
			us->us_EventUsed = 0;
			break;
		}

		count = MIN( remaining, length - us->us_EventUsed );

		memcpy( us->us_EventData + us->us_EventUsed, data, count );

		us->us_EventUsed += count;
		data += count;
		remaining -= count;

		if ( us->us_EventUsed == length )
		{
			type = us->us_EventData[4] | ( (U16) us->us_EventData[5] << 8 );
			code = us->us_EventData[6] | ( (U16) us->us_EventData[7] << 8 );
			us->us_EventUsed = 0;

			if ( ( type == CTYPE_Event ) && ( length >= 16 ) &&
				( ( code == PTP_Event_StoreAdded ) || ( code == PTP_Event_StoreRemoved ) ) )
			{
				memcpy( & storage, us->us_EventData + 12, sizeof( storage ) );

				MYINFO( "PTP-USB : Storage event $%04lx for $%08lx", (U32) code, LE_SWAP32( storage ) );

				changed = TRUE;
			}
		}
	}

	if ( changed )
	{
		ObtainSemaphore( & camera->cin_Gate );

		camera->cin_StorageGeneration++;

		ReleaseSemaphore( & camera->cin_Gate );
	}

	return ( TRUE );
}

// --
