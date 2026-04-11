
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "HID.h"

// --

SEC_CODE void Input_Mouse_Scroll( struct USBBase *usbbase, struct HID_ReportNode *rn, S32 v )
{
struct InputEvent *Event;
struct IOStdReq *ioreq;

	TASK_NAME_ENTER( "Input_Mouse_Scroll" );

	// --

	if ( v )
	{
		Event = & rn->Input_Event;
		Event->ie_NextEvent	= NULL;
		Event->ie_Class		= IECLASS_MOUSEWHEEL;
		Event->ie_SubClass	= 0;
		Event->ie_Code		= IECODE_NOBUTTON;
		Event->ie_Qualifier	= IEQUALIFIER_RELATIVEMOUSE | usbbase->usb_HID_Qualifiers;
		Event->ie_X			= 0;
		Event->ie_Y			= -v;

		ioreq = & rn->Input_IOReq;
		ioreq->io_Command	= IND_ADDEVENT;
		ioreq->io_Data		= Event;
		ioreq->io_Length	= sizeof( struct InputEvent );

		IO_DO( ioreq );
	}

	// --

	TASK_NAME_LEAVE();
}

// --
