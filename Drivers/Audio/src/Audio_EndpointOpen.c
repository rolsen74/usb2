
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE static S32
SameID( const char *left, const char *right )
{
	U32 index;

	for( index = 0; index < AUDIO_ENDPOINT_ID_SIZE; index++ )
	{
		if ( left[index] != right[index] )
		{
			return ( FALSE );
		}

		if ( ! left[index] )
		{
			return ( TRUE );
		}
	}

	return ( FALSE );
}

// --

// Caller holds the device semaphore across lookup and reservation.
SEC_CODE S32
Audio_EndpointFind( struct AudioBase *base, const char *id, struct AudioUnit **selected )
{
	struct AudioUnit *unit;

	U32 index;

	*selected = NULL;

	if ( ! id[0] )
	{
		return ( FALSE );
	}

	for( index = 0; index < AUDIO_MAX_UNITS; index++ )
	{
		unit = base->Slots[index].Unit;

		if ( ( ! unit ) || ( unit->Detached ) )
		{
			continue;
		}

		if ( ( SameID( unit->Endpoint.ID, id ) ) || ( SameID( unit->PortID, id ) ) )
		{
			if ( *selected )
			{
				*selected = NULL;

				return ( FALSE );
			}

			*selected = unit;
		}
	}

	return ( *selected != NULL );
}

// --

SEC_CODE S32
Audio_EndpointOpen( struct AudioBase *base, struct IORequest *ioreq, struct AudioUnit **selected )
{
	struct AudioEndpointRequest *request;

	if ( ioreq->io_Message.mn_Length < sizeof( *request ) )
	{
		return ( IOERR_BADLENGTH );
	}

	request = (PTR) ioreq;

	if ( request->Version != AUDIO_ENDPOINT_VERSION )
	{
		return ( IOERR_NOCMD );
	}

	if ( ( ! Audio_EndpointFind( base, request->ID, selected ) ) ||
		( request->Direction != (*selected)->Endpoint.Direction ) )
	{
		*selected = NULL;

		return ( IOERR_OPENFAIL );
	}

	return ( 0 );
}

// --
