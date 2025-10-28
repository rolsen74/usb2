
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
// Safe String Compare, can handle NULL Pointers
// case-insensitive string comparison

SEC_CODE S32 __Misc_StrINCaseCompare( struct USBBase *usbbase, STR str1, STR str2, S32 len )
{
S32 retval;

	TASK_NAME_ENTER( "__Misc_StrINCaseCompare" );

	if (( ! str1 ) || ( ! str2 ) || ( len <= 0 ))
	{
		retval = -1;
	}
	else
	{
		retval = usbbase->usb_IUtility->Strnicmp( str1, str2, len );
	}

	TASK_NAME_LEAVE();

	return( retval );
}

// --
