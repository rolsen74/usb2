
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

static S32
ReadBulk( struct USB_Struct *us, U32 *actual )
{
	struct USB2_IORequest *io;
	struct USB2_EPResource *input;

	U32 empty;

	input = us->us_Res_BulkIn;
	io = input->IORequests[0];

	// An exact-packet data phase can be followed by a zero-length packet.
	for( empty = 0; empty < 2; empty++ )
	{
		io->io_Command = CMD_READ;
		io->io_Data = input->Buffers[0];
		io->io_Length = input->BufferSize;
		io->io_TimeOut = 5000000;

		DoIO( (PTR) io );

		if ( io->io_Error == USB2Err_Device_Detached )
		{
			us->us_Detached = TRUE;
		}

		if ( ( io->io_Error ) || ( io->io_Actual > input->BufferSize ) )
		{
			return ( FALSE );
		}

		if ( io->io_Actual )
		{
			*actual = io->io_Actual;

			return ( TRUE );
		}
	}

	return ( FALSE );
}

// --

void
_usb_Transaction( struct USB_Struct *us, struct CommandMessage *request )
{
	struct PTP_Container *container;

	U8 *buffer;
	U32 index;
	U32 transaction;
	U32 available;
	U32 length;
	U32 remaining;
	U32 count;
	U16 code;
	S32 got_data;
	S32 data_error;

	request->cm_Error = ERROR_DEVICE_NOT_MOUNTED;
	request->cm_Response = 0;
	request->cm_Actual = 0;
	request->cm_ReplyCount = 0;

	if ( ( ! us->us_Running ) || ( us->us_Detached ) )
	{
		return;
	}

	if ( ( request->cm_ParameterCount > sizeof( request->cm_Parameters ) / sizeof( request->cm_Parameters[0] ) ) ||
		( request->cm_SendData && ( request->cm_Limit > PTP_MAX_UPLOAD ) ) )
	{
		request->cm_Error = ERROR_BAD_NUMBER;

		return;
	}

	transaction = us->us_TransactionID;
	length = 12 + request->cm_ParameterCount * 4;
	got_data = FALSE;
	data_error = 0;

	if ( us->us_Session && ( ! ++us->us_TransactionID ) )
	{
		us->us_TransactionID = 1;
	}

	container = us->us_Res_BulkOut->Buffers[0];
	container->Length = LE_SWAP32( length );
	container->Type = LE_SWAP16( CTYPE_Command );
	container->Code = LE_SWAP16( request->cm_Operation );
	container->TransID = LE_SWAP32( transaction );

	for( index = 0; index < request->cm_ParameterCount; index++ )
	{
		container->Parms[index] = LE_SWAP32( request->cm_Parameters[index] );
	}

	if ( ! _usb_WriteBulk( us, length, FALSE ) )
	{
		goto broken;
	}

	if ( ( request->cm_SendData ) && ( ! _usb_SendData( us, request, transaction ) ) )
	{
		goto broken;
	}

	if ( ! ReadBulk( us, & available ) )
	{
		goto broken;
	}

	buffer = us->us_Res_BulkIn->Buffers[0];
	container = (PTR) buffer;

	if ( available < 12 )
	{
		goto broken;
	}

	if ( LE_SWAP16( container->Type ) == CTYPE_Data )
	{
		if ( ( ! request->cm_WantData ) || ( LE_SWAP16( container->Code ) != request->cm_Operation ) ||
			( LE_SWAP32( container->TransID ) != transaction ) )
		{
			goto broken;
		}

		length = LE_SWAP32( container->Length );

		if ( ( length < 12 ) || ( length - 12 > request->cm_Limit ) )
		{
			goto broken;
		}

		got_data = TRUE;
		remaining = length - 12;
		buffer += 12;
		available -= 12;

		if ( request->cm_Allocate && remaining )
		{
			request->cm_Data = AllocVecTags( remaining, AVT_Type, MEMF_SHARED, TAG_END );

			if ( ! request->cm_Data )
			{
				data_error = ERROR_NO_FREE_STORE;
			}
		}

		while( remaining )
		{
			if ( ! available )
			{
				if ( ! ReadBulk( us, & available ) )
				{
					goto broken;
				}

				buffer = us->us_Res_BulkIn->Buffers[0];
			}

			count = MIN( available, remaining );

			if ( ! data_error )
			{
				if ( request->cm_Output )
				{
					if ( Write( request->cm_Output, buffer, count ) != (S32) count )
					{
						data_error = IoErr();

						if ( ! data_error )
						{
							data_error = ERROR_DISK_FULL;
						}
					}
				}
				else
				{
					memcpy( request->cm_Data + request->cm_Actual, buffer, count );
				}
			}

			request->cm_Actual += count;
			remaining -= count;
			available -= count;
			buffer += count;
		}

		if ( ! available )
		{
			if ( ! ReadBulk( us, & available ) )
			{
				goto broken;
			}

			buffer = us->us_Res_BulkIn->Buffers[0];
		}

		container = (PTR) buffer;
	}

	if ( available < 12 )
	{
		goto broken;
	}

	length = LE_SWAP32( container->Length );

	if ( ( length != available ) || ( length > sizeof( *container ) ) || ( length & 3 ) ||
		( LE_SWAP16( container->Type ) != CTYPE_Response ) ||
		( LE_SWAP32( container->TransID ) != transaction ) )
	{
		goto broken;
	}

	code = LE_SWAP16( container->Code );
	request->cm_Response = code;
	request->cm_ReplyCount = MIN( ( length - 12 ) / 4, sizeof( request->cm_Reply ) / sizeof( request->cm_Reply[0] ) );

	for( index = 0; index < request->cm_ReplyCount; index++ )
	{
		request->cm_Reply[index] = LE_SWAP32( container->Parms[index] );
	}

	switch( code )
	{
		case PTP_Response_OK:
			if ( request->cm_WantData && ! got_data )
			{
				goto broken;
			}

			request->cm_Error = data_error;
			break;
		case PTP_Response_Unsupported:
			request->cm_Error = ERROR_ACTION_NOT_KNOWN;
			break;
		case PTP_Response_InvalidHandle:
		case PTP_Response_InvalidParent:
		case PTP_Response_InvalidStorage:
			request->cm_Error = ERROR_OBJECT_NOT_FOUND;
			break;
		case PTP_Response_StoreFull:
			request->cm_Error = ERROR_DISK_FULL;
			break;
		case PTP_Response_StoreReadOnly:
			request->cm_Error = ERROR_DISK_WRITE_PROTECTED;
			break;
		case PTP_Response_WriteProtected:
		case PTP_Response_AccessDenied:
			request->cm_Error = request->cm_SendData ? ERROR_WRITE_PROTECTED : ERROR_READ_PROTECTED;
			break;
		case PTP_Response_InvalidFormat:
			request->cm_Error = ERROR_OBJECT_WRONG_TYPE;
			break;
		case PTP_Response_Incomplete:
			request->cm_Error = ERROR_SEEK_ERROR;
			break;
		case PTP_Response_StoreUnavailable:
			request->cm_Error = ERROR_DEVICE_NOT_MOUNTED;
			break;
		case PTP_Response_Busy:
			request->cm_Error = ERROR_OBJECT_IN_USE;
			break;
		default:
			request->cm_Error = request->cm_SendData ? ERROR_WRITE_PROTECTED : ERROR_READ_PROTECTED;
			break;
	}

	return;

broken:

	// A failed or malformed USB transaction leaves the session out of sync.
	us->us_Running = FALSE;
}

// --
