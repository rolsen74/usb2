
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#include "usb2_all.h"
#include "MSD.h"

// --

SEC_CODE U32 MSD_Manager_Release( struct DeviceManagerInterface *Self )
{
	TASK_NAME_ENTER( "MSD : MSD_Manager_AbortIO" );

	Self->Data.RefCount--;

	TASK_NAME_LEAVE();
	return(	Self->Data.RefCount );
}
