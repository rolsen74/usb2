
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
Wireless_StartAuto( U32 adapter, U32 unit )
{
	struct USB2WiFiIFace *wifi;
	struct Library *base;

	base = OpenLibrary( USB2WIFI_NAME, USB2WIFI_VERSION );

	if ( ! base )
	{
		return;
	}

	if ( ( base->lib_Version > USB2WIFI_VERSION ) || ( base->lib_Revision >= USB2WIFI_STARTUP_REVISION ) )
	{
		wifi = (struct USB2WiFiIFace *) GetInterface( base, "main", 1, NULL );

		if ( wifi )
		{
			wifi->StartAuto( adapter, unit );

			DropInterface( (struct Interface *) wifi );
		}
	}

	CloseLibrary( base );
}

// --
