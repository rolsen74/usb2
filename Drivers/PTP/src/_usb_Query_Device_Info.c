
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

void
_usb_Query_Device_Info( struct USB_Struct *us )
{
	struct CommandMessage request;

	const U8 *data;
	char *name;
	U32 remaining;
	U32 field;
	U32 count;
	U32 bytes;
	U32 index;
	U16 code;
	S32 info;
	S32 send;

	memset( & request, 0, sizeof( request ) );

	request.cm_Operation = PTP_Get_Device_Info;
	request.cm_Allocate = TRUE;
	request.cm_WantData = TRUE;
	request.cm_Limit = PTP_METADATA_LIMIT;

	_usb_Transaction( us, & request );

	data = request.cm_Data;
	remaining = request.cm_Actual;
	name = us->us_CameraNode.cin_Name;
	name[0] = 0;
	info = FALSE;
	send = FALSE;

	if ( ( request.cm_Error ) || ( remaining < 11 ) )
	{
		goto done;
	}

	// Skip the fixed header, vendor extension string and FunctionalMode.
	bytes = 11 + 2 * data[8];

	if ( bytes > remaining )
	{
		goto done;
	}

	data += bytes;
	remaining -= bytes;

	// Operations, events, properties, capture formats and image formats.
	for( field = 0; field < 5; field++ )
	{
		if ( remaining < sizeof( count ) )
		{
			goto done;
		}

		memcpy( & count, data, sizeof( count ) );

		count = LE_SWAP32( count );
		data += sizeof( count );
		remaining -= sizeof( count );

		if ( count > remaining / 2 )
		{
			goto done;
		}

		if ( ! field )
		{
			for( index = 0; index < count * 2; index += 2 )
			{
				code = data[index] | ( (U16) data[index + 1] << 8 );
				info |= code == PTP_Send_Object_Info;
				send |= code == PTP_Send_Object;
				us->us_CameraNode.cin_CanReadStorageInfo |= code == PTP_Get_Storage_Info;
			}

			us->us_CameraNode.cin_CanUpload = info && send;
		}

		data += count * 2;
		remaining -= count * 2;
	}

	if ( ! remaining )
	{
		goto done;
	}

	// Skip Manufacturer. The following PTP string is Model.
	bytes = 1 + 2 * data[0];

	if ( bytes >= remaining )
	{
		goto done;
	}

	data += bytes;
	remaining -= bytes;

	_ptp_Read_Name( data, remaining, name, sizeof( us->us_CameraNode.cin_Name ) );

done:

	FreeVec( request.cm_Data );
}

// --
