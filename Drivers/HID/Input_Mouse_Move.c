
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

SEC_CODE void Input_Mouse_Move( struct USBBase *usbbase, struct HID_ReportNode *rn, S32 x, S32 y )
{
struct InputEvent *Event;
struct IOStdReq *ioreq;

	TASK_NAME_ENTER( "Input_Mouse_Move" );

	// --

	if (( x ) || ( y ))
	{
		Event = & rn->Input_Event;
		Event->ie_NextEvent	= NULL;
		Event->ie_Class		= IECLASS_RAWMOUSE;
		Event->ie_SubClass	= 0;
		Event->ie_Code		= IECODE_NOBUTTON;
		Event->ie_Qualifier	= IEQUALIFIER_RELATIVEMOUSE | usbbase->usb_HID_Qualifiers;
		Event->ie_X			= x;
		Event->ie_Y			= y;

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
