
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef DRIVERS_MEDIA_PUBLISHDEVICE_H
#define DRIVERS_MEDIA_PUBLISHDEVICE_H

#include <interfaces/usb2media.h>

// --

static inline BOOL
PublishMediaDevice( struct Library *backend )
{
	struct USB2MediaIFace *media;
	struct IORequest request;

	BOOL registered;

	request = (struct IORequest) { 0 };
	request.io_Message.mn_Length = sizeof( request );
	registered = FALSE;

	if ( OpenDevice( backend->lib_Node.ln_Name, USB2_MEDIA_QUERY_UNIT, & request, 0 ) )
	{
		// Older stacks create the public device when the class driver attaches.
		AddDevice( (struct Device *) backend );

		return ( TRUE );
	}

	media = (PTR) GetInterface( (PTR) request.io_Device, "media", 1, NULL );

	if ( media )
	{
		registered = media->RegisterBackend( (struct Device *) backend );

		DropInterface( (PTR) media );
	}

	CloseDevice( & request );

	return ( registered );
}

#endif
