
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
_usb_SendData( struct USB_Struct *us, struct CommandMessage *request, U32 transaction )
{
	struct USB2_EPResource *output;
	struct PTP_Container *container;

	U8 *buffer;
	U32 header;
	U32 count;
	U32 remaining;

	output = us->us_Res_BulkOut;
	container = output->Buffers[0];
	container->Length = LE_SWAP32( 12 + request->cm_Limit );
	container->Type = LE_SWAP16( CTYPE_Data );
	container->Code = LE_SWAP16( request->cm_Operation );
	container->TransID = LE_SWAP32( transaction );

	header = 12;
	remaining = request->cm_Limit;

	do
	{
		buffer = (U8 *) container + header;
		count = MIN( remaining, output->BufferSize - header );

		if ( count )
		{
			if ( request->cm_Input )
			{
				if ( Read( request->cm_Input, buffer, count ) != (S32) count )
				{
					request->cm_Error = IoErr();

					if ( ! request->cm_Error )
					{
						request->cm_Error = ERROR_SEEK_ERROR;
					}

					return ( FALSE );
				}
			}
			else
			{
				memcpy( buffer, request->cm_Data + request->cm_Actual, count );
			}
		}

		remaining -= count;

		if ( ! _usb_WriteBulk( us, header + count, ! remaining ) )
		{
			return ( FALSE );
		}

		request->cm_Actual += count;
		header = 0;
	}
	while( remaining );

	return ( TRUE );
}

// --
