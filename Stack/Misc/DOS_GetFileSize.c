
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#include "usb2_all.h"

// --

SEC_CODE S32 __Misc_GetFileSize( struct USBBase *usbbase, S32 handle )
{
S32 size;

	TASK_NAME_ENTER( "__DOS_GetFileSize" );

	if ( handle )
	{
		size = (S32) usbbase->usb_IDOS->GetFileSize( handle );
	}
	else
	{
		size = -1;
	}

	TASK_NAME_LEAVE();

	return( size );
}

// --
