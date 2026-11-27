/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef DRIVERS_AUDIO_DEVICE_H
#define DRIVERS_AUDIO_DEVICE_H

#include <devices/usb2audio.h>
#include <exec/libraries.h>

#define AUDIO_DEVICE_VERSION 1
#define AUDIO_DEVICE_REVISION 9
#define AUDIO_ENDPOINT_BACKEND_REVISION 3
#define AUDIO_DEVICE_ID AUDIO_DEVICE_NAME " 1.9 (27.11.2026)"

static inline U32
Audio_QueuedWriteDepth( const struct Library *device )
{
	if ( LIB_IS_AT_LEAST( device, AUDIO_DEVICE_VERSION, AUDIO_THREE_WRITES_REVISION ) )
	{
		return ( AUDIO_MAX_QUEUED_WRITES );
	}

	return ( LIB_IS_AT_LEAST( device, AUDIO_DEVICE_VERSION, 1 ) ? 2 : 1 );
}

#endif
