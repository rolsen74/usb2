
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

SEC_CODE void __Misc_ReleaseInterface( struct USBBase *usbbase, PTR ifc )
{
	TASK_NAME_ENTER( "__EXEC_ReleaseInterface" );

	if ( ifc )
	{
		usbbase->usb_IExec->DropInterface( ifc );
	}

	TASK_NAME_LEAVE();
}

// --
