
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

SEC_CODE S32 __Misc_ReadFile( struct USBBase *usbbase, S32 handle, STR mem, S32 size )
{
S32 len;

	TASK_NAME_ENTER( "__DOS_ReadFile" );

	if (( handle ) && ( mem ) && ( size > 0 ))
	{
		len = usbbase->usb_IDOS->Read( handle, mem, size );
	}
	else
	{
		len = -1;
	}

	TASK_NAME_LEAVE();

	return( len );
}

// --
