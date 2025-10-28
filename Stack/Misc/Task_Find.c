
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

SEC_CODE PTR __Task_Find( struct USBBase *usbbase )
{
struct ExecIFace *IExec;
PTR retval;

	IExec = usbbase->usb_IExec;
	retval = (PTR) IExec->FindTask( NULL );

	return( retval );
}

// --
