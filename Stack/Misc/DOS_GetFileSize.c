
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE S32
__Misc_GetFileSize( struct USBBase *usbbase, S32 handle )
{
	S64 size;

	TASK_NAME_ENTER( "__DOS_GetFileSize" );

	size = -1;

	if ( handle )
	{
		size = usbbase->usb_IDOS->GetFileSize( handle );

		if ( ( size < 0 ) || ( size > 0x7fffffff ) )
		{
			size = -1;
		}
	}

	TASK_NAME_LEAVE();

	return ( (S32)size );
}

// --
