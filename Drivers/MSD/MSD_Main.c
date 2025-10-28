
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
#if 1

SEC_CODE void MSD_Main( struct USBBase *usbbase, struct MSDDevice *msddev )
{
	TASK_NAME_ENTER( "MSD : MSD_Main" );
	USBERROR( "MSD : MSD_Main : Enter" );

	// --
	// Normal running loop
	MSD_Main_1_Normal( usbbase, msddev );

	// --
	// Signal subtasks to shutdown
//	__Signal( 	usbbase, in );

	// --
	// Stopping loop, wait for Resources to be released
	MSD_Main_2_Stopping( usbbase, msddev );

	// --
	// Start shutdown
//	__Shutdown( usbbase, in );

	// --
	// -- Exit

	USBERROR( "MSD : MSD_Main : Leave" );
	TASK_NAME_LEAVE();
}

#endif
// --
