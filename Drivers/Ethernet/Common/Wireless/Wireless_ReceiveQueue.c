
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <proto/exec.h>
#include "Wireless_ReceiveQueue.h"

// --

void
Wireless_SubmitRX( struct USB2_EPResource *resource, U32 *pending )
{
	struct USB2_IORequest *request;

	U32 index;
	U32 bit;

	for( index = 0; index < WIRELESS_RX_REQUEST_COUNT; index++ )
	{
		bit = 1U << index;

		if ( *pending & bit )
		{
			continue;
		}

		request = resource->IORequests[index];
		request->io_Command = CMD_READ;
		request->io_Data = resource->Buffers[index];
		request->io_Length = resource->BufferSize;
		request->io_Actual = 0;
		*pending |= bit;

		SendIO( (PTR) request );
	}
}

// --

struct USB2_IORequest *
Wireless_TakeRX( struct USB2_EPResource *resource, U32 *pending )
{
	struct USB2_IORequest *request;

	U32 index;

	// Read completed transfers in reply order, independent of buffer index.
	request = (PTR) GetMsg( resource->MsgPort );

	if ( ! request )
	{
		return ( NULL );
	}

	for( index = 0; index < WIRELESS_RX_REQUEST_COUNT; index++ )
	{
		if ( resource->IORequests[index] == request )
		{
			*pending &= ~( 1U << index );

			return ( request );
		}
	}

	return ( NULL );
}

// --

void
Wireless_StopRX( struct USB2_EPResource *resource, U32 *pending )
{
	struct IORequest *request;

	U32 index;

	// Abort the entire queue before waiting so a pending read cannot survive a retune.
	for( index = 0; index < WIRELESS_RX_REQUEST_COUNT; index++ )
	{
		if ( *pending & ( 1U << index ) )
		{
			request = (PTR) resource->IORequests[index];

			if ( ! CheckIO( request ) )
			{
				AbortIO( request );
			}
		}
	}

	for( index = 0; index < WIRELESS_RX_REQUEST_COUNT; index++ )
	{
		if ( *pending & ( 1U << index ) )
		{
			WaitIO( (PTR) resource->IORequests[index] );
		}
	}

	*pending = 0;
}

// --
