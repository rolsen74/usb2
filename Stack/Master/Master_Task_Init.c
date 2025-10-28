
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#include "usb2_all.h"
#include "Master.h"

// --

SEC_CODE static S32 __myTask_Init( struct USBBase *usbbase, struct intern *in )
{
S32 retval;

	retval = FALSE;
	USBINFO( "__myTask_Init (Master)" );

	// --

	if ( ! __myTask_Def_Init( usbbase, in ))
	{
		USBERROR( "__myTask_Init (Master) : 1 : Error" );
		goto bailout;
	}

	// --

	retval = TRUE;

bailout:

	return( retval );
}

// --
