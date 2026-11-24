
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

	camera = & us->us_CameraNode;
	data = io->io_Data;
	remaining = io->io_Actual;

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

			_usb_Invalidate_Storage( us, FALSE );

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
				memcpy( &storage, us->us_EventData + 12, sizeof( storage ) );

				storage = LE_SWAP32( storage );

				ObtainSemaphore( &camera->cin_Gate );

				camera->cin_StorageGeneration++;

				if ( code == PTP_Event_StoreRemoved )
				{
					if ( camera->cin_RemovalCount == PTP_MAX_REMOVALS )
					{
						camera->cin_InvalidateStores = TRUE;
						camera->cin_RemovalCount = 0;
					}
					else
					{
						camera->cin_Removals[camera->cin_RemovalCount++] = storage;
					}
				}

				ReleaseSemaphore( &camera->cin_Gate );
			}
		}
	}

	return ( TRUE );
}

// --

void
_usb_Invalidate_Storage( struct USB_Struct *us, S32 event_loss )
{
	struct CameraInfoNode *camera;

	camera = &us->us_CameraNode;

	ObtainSemaphore( &camera->cin_Gate );

	camera->cin_StorageGeneration++;
	camera->cin_InvalidateStores = TRUE;
	camera->cin_RemovalCount = 0;

	if ( event_loss )
	{
		camera->cin_EventsReliable = FALSE;
	}

	ReleaseSemaphore( &camera->cin_Gate );
}

// --
