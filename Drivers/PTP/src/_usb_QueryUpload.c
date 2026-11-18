
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
_usb_QueryUpload( struct USB_Struct *us )
{
	struct CommandMessage request;

	U8 *data;
	U32 offset;
	U32 count;
	U32 operation;
	S32 info;
	S32 send;

	memset( & request, 0, sizeof( request ) );

	request.cm_Operation = PTP_Get_Device_Info;
	request.cm_Allocate = TRUE;
	request.cm_WantData = TRUE;
	request.cm_Limit = PTP_METADATA_LIMIT;

	_usb_Transaction( us, & request );

	data = request.cm_Data;

	if ( ( ! request.cm_Error ) && ( request.cm_Actual >= 15 ) )
	{
		// Skip the extension description and FunctionalMode to reach OperationsSupported.
		offset = 11 + 2 * data[8];

		if ( offset <= request.cm_Actual - 4 )
		{
			count = data[offset] | ( (U32) data[offset + 1] << 8 ) |
				( (U32) data[offset + 2] << 16 ) | ( (U32) data[offset + 3] << 24 );

			offset += 4;
			info = FALSE;
			send = FALSE;

			if ( count <= ( request.cm_Actual - offset ) / 2 )
			{
				while( count-- )
				{
					operation = data[offset] | ( (U32) data[offset + 1] << 8 );
					info |= operation == PTP_Send_Object_Info;
					send |= operation == PTP_Send_Object;
					offset += 2;
				}

				us->us_CameraNode.cin_CanUpload = info && send;
			}
		}
	}

	FreeVec( data );
}

// --
