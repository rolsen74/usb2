
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

SEC_CODE void MSD_Device_Free( struct USBBase *usbbase UNUSED, struct MSDDevice *msddev UNUSED )
{
	TASK_NAME_ENTER( "MSD_Device_Free" );
	USBDEBUG( "MSD : MSD_Device_Free" );

	// --


	// --

	TASK_NAME_LEAVE();
}

// --
