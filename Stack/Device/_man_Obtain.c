
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

SEC_CODE static U32 _manager_Obtain( struct Interface *Self )
{
	TASK_NAME_ENTER( "USB : _manager_Obtain" );

	Self->Data.RefCount++;

	TASK_NAME_LEAVE();
	return( Self->Data.RefCount );
}

// --
