
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
// Safe String Length, can handle NULL Pointer

SEC_CODE U32 __Misc_StrLength( struct USBBase *usbbase UNUSED, STR txt )
{
STR str;
U32 len;

	TASK_NAME_ENTER( "__Misc_StrLength" );

	len = 0;

	if ( ! txt )
	{
		goto bailout;
	}

	str = txt;

	while( *str )
	{
		str++;
	}

	len = str - txt;

bailout:

	TASK_NAME_LEAVE();

	return( len );
}

// --
