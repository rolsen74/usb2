
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

SEC_CODE void MSD_Disk_Free( struct USBBase *usbbase UNUSED, struct MSDDisk *msddisk UNUSED )
{
	TASK_NAME_ENTER( "MSD_Disk_Free" );
	USBDEBUG( "MSD : MSD_Disk_Free" );

	// --


	// --

	TASK_NAME_LEAVE();
}

// --
