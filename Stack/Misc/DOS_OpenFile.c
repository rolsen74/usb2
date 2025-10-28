
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

SEC_CODE S32 __Misc_OpenFile( struct USBBase *usbbase, STR filename, U32 filemode )
{
S32 handle;

	TASK_NAME_ENTER( "__DOS_OpenFile" );

	if ( filename )
	{
		handle = usbbase->usb_IDOS->Open( filename, filemode );
	}
	else
	{
		handle = 0;
	}

	TASK_NAME_LEAVE();

	return( handle );
}

// --
