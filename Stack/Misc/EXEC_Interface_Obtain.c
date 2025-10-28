
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

SEC_CODE PTR __Misc_ObtainInterface( struct USBBase *usbbase, PTR base, STR name, U32 ver )
{
PTR ifc;

	TASK_NAME_ENTER( "__EXEC_ObtainInterface" );

	if ( base )
	{
		ifc = usbbase->usb_IExec->GetInterface( base, name, ver, NULL );
	}
	else
	{
		ifc = NULL;
	}

	TASK_NAME_LEAVE();

	return( ifc );
}

// --
