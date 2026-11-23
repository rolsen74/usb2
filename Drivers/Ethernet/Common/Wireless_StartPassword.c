
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <proto/exec.h>
#include <usb2wifi.h>
#include "WiFiCommand.h"

// --

void
Wireless_StartPassword( struct IOSana2Req *request, U32 adapter, U32 unit )
{
	struct USB2WiFiIFace *wifi;
	struct Library *base;

	S32 started;

	if ( ( ! request->ios2_Data ) || ( request->ios2_DataLength != sizeof( struct USB2WiFiPrompt ) ) )
	{
		request->ios2_Req.io_Error = IOERR_BADLENGTH;

		return;
	}

	base = OpenLibrary( USB2WIFI_NAME, USB2WIFI_VERSION );

	if ( ( ! base ) || ( base->lib_Version == USB2WIFI_VERSION && base->lib_Revision < USB2WIFI_PROMPT_REVISION ) )
	{
		if ( base )
		{
			CloseLibrary( base );
		}

		request->ios2_Req.io_Error = IOERR_OPENFAIL;

		return;
	}

	wifi = (struct USB2WiFiIFace *) GetInterface( base, "main", 1, NULL );

	if ( wifi )
	{
		// The pending requester belongs to the caller until DOS signals its exit.
		if ( ( base->lib_Version > USB2WIFI_VERSION ) || ( base->lib_Revision >= USB2WIFI_TITLED_PROMPT_REVISION ) )
		{
			started = wifi->StartPromptForUnit( request->ios2_Data, adapter, unit );
		}
		else
		{
			started = wifi->StartPrompt( request->ios2_Data );
		}

		if ( ! started )
		{
			request->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
		}

		DropInterface( (struct Interface *) wifi );
	}
	else
	{
		request->ios2_Req.io_Error = IOERR_OPENFAIL;
	}

	CloseLibrary( base );
}

// --
