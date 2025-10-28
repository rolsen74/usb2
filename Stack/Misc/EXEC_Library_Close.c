
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

SEC_CODE void __Misc_CloseLibrary( struct USBBase *usbbase, PTR base )
{
	TASK_NAME_ENTER( "__EXEC_CloseLibrary" );

	if ( base )
	{
		usbbase->usb_IExec->CloseLibrary( base );
	}

	TASK_NAME_LEAVE();
}

// --
