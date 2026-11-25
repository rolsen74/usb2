/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE S32 Audio_Manager_Open( struct DeviceManagerInterface *Self, struct IORequest *ioreq, U32 unitnr, U32 flags UNUSED )
{
struct AudioBase *base;
struct AudioUnit *unit;

S32 error;
U32 index;

	base = (PTR) Self->Data.LibBase;
	unit = NULL;
	error = 0;

	ObtainSemaphore( & base->Semaphore );

	if ( unitnr == USB2_MEDIA_QUERY_UNIT )
	{
		ioreq->io_Unit = NULL;
	}
	else if ( unitnr == AUDIO_UNIT_ENDPOINT )
	{
		error = Audio_EndpointOpen( base, ioreq, & unit );
	}
	else if ( unitnr == AUDIO_UNIT_FIRST_OUTPUT )
	{
		for( index = AUDIO_PLAYBACK_UNIT_BASE; index < AUDIO_MAX_UNITS; index++ )
		{
			unit = base->Units[index];

			if ( ( unit ) && ( ! unit->Detached ) &&
				( unit->Endpoint.DefaultFormat.Encoding == AUDIO_ENCODING_PCM_S16LE ) &&
				( ( unit->Endpoint.DefaultFormat.Channels == 1 ) || ( unit->Endpoint.DefaultFormat.Channels == 2 ) ) )
			{
				break;
			}

			unit = NULL;
		}
	}
	else if ( unitnr < AUDIO_MAX_UNITS )
	{
		unit = base->Units[unitnr];
	}

	if ( error )
	{
		ioreq->io_Error = error;
		ioreq->io_Device = NULL;
	}
	else if ( unitnr == USB2_MEDIA_QUERY_UNIT )
	{
		base->Library.lib_OpenCnt++;
		ioreq->io_Error = 0;
	}
	else if ( ( ! unit ) || ( unit->Detached ) )
	{
		AUDIO_TRACE( "open unit %lu: no registered stream\n", unitnr );

		ioreq->io_Error = IOERR_OPENFAIL;
		ioreq->io_Device = NULL;
	}
	else if ( unit->OpenCount )
	{
		AUDIO_TRACE( "open unit %lu: already in use\n", unitnr );

		ioreq->io_Error = IOERR_UNITBUSY;
		ioreq->io_Device = NULL;
	}
	else if ( ioreq->io_Message.mn_Length < sizeof( struct IOStdReq ) )
	{
		AUDIO_TRACE( "open unit %lu: I/O request too short (%lu bytes)\n",
			unitnr, (U32) ioreq->io_Message.mn_Length );

		ioreq->io_Error = IOERR_BADLENGTH;
		ioreq->io_Device = NULL;
	}
	else
	{
		base->Library.lib_OpenCnt++;
		unit->OpenCount++;
		ioreq->io_Unit = (PTR) unit;
		ioreq->io_Error = 0;
	}

	ReleaseSemaphore( & base->Semaphore );

	return ( ioreq->io_Error );
}

// --
