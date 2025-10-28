
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

SEC_CODE PTR MSD_Manager_Expunge( struct DeviceManagerInterface *Self UNUSED )
{
	TASK_NAME_ENTER( "MSD : MSD_Manager_Expunge" );
	TASK_NAME_LEAVE();
	return(	NULL );
}
