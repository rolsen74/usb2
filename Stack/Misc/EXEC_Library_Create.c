
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

SEC_CODE PTR __Misc_CreateLibrary( struct USBBase *usbbase, PTR taglist )
{
PTR retval;

	if ( taglist )
	{
		retval = usbbase->usb_IExec->CreateLibrary( taglist );
	}
	else
	{
		retval = NULL;
	}

	return(	retval );
}

// --
