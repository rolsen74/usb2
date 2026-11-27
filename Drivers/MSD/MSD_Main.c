
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "MSD.h"

// --
#if 1

SEC_CODE S32
MSD_Main( struct USBBase *usbbase, struct MSDDevice *msddev )
{
	S32 promoted;

	TASK_NAME_ENTER( "MSD : MSD_Main" );
	USBERROR( "MSD : MSD_Main : Enter" );

	// --
	// Normal running loop
	promoted = MSD_Main_1_Normal( usbbase, msddev );

	// --
	// -- Exit

	USBERROR( "MSD : MSD_Main : Leave" );
	TASK_NAME_LEAVE();

	return ( promoted );
}

#endif
// --
