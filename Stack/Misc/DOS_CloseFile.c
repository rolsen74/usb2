
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

SEC_CODE void __Misc_CloseFile( struct USBBase *usbbase, S32 handle )
{
	TASK_NAME_ENTER( "__DOS_CloseFile" );

	if ( handle )
	{
		usbbase->usb_IDOS->Close( handle );
	}

	TASK_NAME_LEAVE();
}

// --
