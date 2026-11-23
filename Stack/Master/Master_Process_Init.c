
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "Master.h"

// --

SEC_CODE S32 __myProcess_Init( struct USBBase *usbbase, struct intern *in )
{
S32 retval;

	retval = FALSE;

	// --

	if ( ! __myTask_Def_Init( usbbase, in ))
	{
		USBERROR( "__myProcess_Init (Master) : 1 : Error" );
		goto bailout;
	}

	retval = TRUE;

bailout:

	return( retval );
}

// --
