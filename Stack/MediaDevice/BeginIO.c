
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "MediaDevice.h"
#include "Drivers/Audio/inc/AudioDevice.h"

// --

SEC_CODE void
MediaDevice_BeginIO( struct DeviceManagerInterface *Self, struct IORequest *request )
{
	struct USB2MediaDevice *base;
	struct DeviceManagerInterface *backend;

	base = (PTR) Self->Data.LibBase;

	if ( request->io_Command == NSCMD_DEVICEQUERY )
	{
		USB2_DeviceQuery( (PTR) request, NSDEVTYPE_UNKNOWN, base->Commands );
	}
	else if ( request->io_Message.mn_Length < sizeof( struct IOStdReq ) )
	{
		request->io_Error = IOERR_BADLENGTH;
	}
	else if ( ( base->Audio ) && ( request->io_Command == AUDIO_CMD_ENUM_ENDPOINTS ) )
	{
		struct IOStdReq *io;
		struct Library *library;

		io = (PTR) request;
		io->io_Actual = 0;
		io->io_Error = IOERR_NOCMD;

		backend = MediaDevice_Backend( base );

		if ( backend )
		{
			library = backend->Data.LibBase;

			if ( ( library->lib_Version > AUDIO_DEVICE_VERSION ) ||
				( ( library->lib_Version == AUDIO_DEVICE_VERSION ) && ( library->lib_Revision >= AUDIO_ENDPOINT_BACKEND_REVISION ) ) )
			{
				backend->BeginIO( request );

				return;
			}
		}
		else if ( io->io_Offset == AUDIO_ENDPOINT_VERSION )
		{
			io->io_Error = 0;
		}
	}
	else if ( request->io_Unit )
	{
		backend = MediaDevice_Backend( base );

		backend->BeginIO( request );

		return;
	}
	else
	{
		((struct IOStdReq *) request)->io_Actual = 0;
		request->io_Error = IOERR_OPENFAIL;
	}

	if ( ! ( request->io_Flags & IOF_QUICK ) )
	{
		base->IExec->ReplyMsg( (PTR) request );
	}
}

// --
