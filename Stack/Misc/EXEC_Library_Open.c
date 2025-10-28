
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

SEC_CODE PTR __Misc_OpenLibrary( struct USBBase *usbbase, STR name, U32 ver )
{
PTR base;

	TASK_NAME_ENTER( "__EXEC_OpenLibrary" );

	if ( name )
	{
		base = usbbase->usb_IExec->OpenLibrary( name, ver );
	}
	else
	{
		base = NULL;
	}

	TASK_NAME_LEAVE();

	return( base );
}

// --
