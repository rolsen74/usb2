
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "MediaDevice.h"

// --

SEC_CODE void
MediaDevice_Open( struct DeviceManagerInterface *Self, struct IORequest *request, U32 number, U32 flags )
{
	struct USB2MediaDevice *base;
	struct DeviceManagerInterface *backend;

	base = (PTR) Self->Data.LibBase;
	request->io_Unit = NULL;
	request->io_Error = IOERR_OPENFAIL;

	if ( request->io_Message.mn_Length >= sizeof( struct IORequest ) )
	{
		if ( number == USB2_MEDIA_QUERY_UNIT )
		{
			request->io_Error = 0;
		}
		else if ( request->io_Message.mn_Length >= sizeof( struct IOStdReq ) )
		{
			backend = MediaDevice_Backend( base );

			if ( backend )
			{
				backend->Open( request, number, flags );
			}
		}
	}

	if ( request->io_Error )
	{
		request->io_Device = NULL;

		return;
	}

	base->IExec->ObtainSemaphore( & base->Lock );

	base->Library.lib_OpenCnt++;
	request->io_Device = (PTR) base;
	request->io_Message.mn_Node.ln_Type = NT_REPLYMSG;

	base->IExec->ReleaseSemaphore( & base->Lock );
}

// --
