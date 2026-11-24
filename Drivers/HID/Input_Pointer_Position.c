
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "HID.h"
#include <intuition/intuition.h>

// --

SEC_CODE void
Input_Pointer_Position( struct USBBase *usbbase, struct HID_ReportNode *rn, U32 tablet, U32 buttons )
{
	struct IENewTablet position;
	struct InputEvent *event;
	struct IOStdReq *input;
	struct TagItem tags[4];

	U32 count;

	if ( ( ! rn->Pointer_RangeX ) || ( ! rn->Pointer_RangeY ) )
	{
		return;
	}

	count = 0;

	MEM_SET( &position, 0, sizeof( position ) );

	position.ient_TabletX = rn->Pointer_X;
	position.ient_TabletY = rn->Pointer_Y;
	position.ient_RangeX = rn->Pointer_RangeX;
	position.ient_RangeY = rn->Pointer_RangeY;

	if ( ( tablet ) && ( rn->Tablet_HasPressure ) )
	{
		tags[count].ti_Tag = TABLETA_Pressure;
		tags[count++].ti_Data = ( rn->Tablet_InRange ) ? rn->Tablet_Pressure : 0;
	}

	if ( ( tablet ) && ( rn->Tablet_HasProximity ) )
	{
		tags[count].ti_Tag = TABLETA_InProximity;
		tags[count++].ti_Data = rn->Tablet_InRange;
	}

	tags[count].ti_Tag = TABLETA_ButtonBits;
	tags[count++].ti_Data = buttons;
	tags[count].ti_Tag = TAG_DONE;
	tags[count].ti_Data = 0;
	position.ient_TagList = tags;

	event = &rn->Input_Event;
	event->ie_NextEvent = NULL;
	event->ie_Class = IECLASS_NEWPOINTERPOS;
	event->ie_SubClass = IESUBCLASS_NEWTABLET;
	event->ie_Code = IECODE_NOBUTTON;
	event->ie_Qualifier = usbbase->usb_HID_Qualifiers;
	event->ie_EventAddress = &position;

	input = &rn->Input_IOReq;
	input->io_Command = IND_WRITEEVENT;
	input->io_Data = event;
	input->io_Length = sizeof( *event );

	IO_DO( input );
}

// --
