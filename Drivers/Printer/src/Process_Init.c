
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

static S32
__myProcess_Init( struct PrinterUnit *unit, struct USB2_DriverMessage *msg )
{
	S32 retval;

	retval = FALSE;

	USBERROR( "Printer : __myProcess_Init" );

	// --

	unit->unit_USB_IOReq_FreeList		= AllocSysObjectTags( ASOT_LIST, TAG_END );

	unit->unit_PAR_IOReq_WaitingList	= AllocSysObjectTags( ASOT_LIST, TAG_END );

	unit->unit_Buffer_FreeList			= AllocSysObjectTags( ASOT_LIST, TAG_END );

	unit->unit_Buffer_WaitingList		= AllocSysObjectTags( ASOT_LIST, TAG_END );

	unit->unit_PRT_Begin_MsgPort			= AllocSysObjectTags( ASOT_PORT, TAG_END );

	unit->unit_PRT_Abort_MsgPort			= AllocSysObjectTags( ASOT_PORT, TAG_END );

	if (( ! unit->unit_USB_IOReq_FreeList )
	||	( ! unit->unit_PAR_IOReq_WaitingList )
	||	( ! unit->unit_Buffer_FreeList )
	||	( ! unit->unit_Buffer_WaitingList )
	||	( ! unit->unit_PRT_Begin_MsgPort )
	||	( ! unit->unit_PRT_Abort_MsgPort ))
	{
		USBERROR( "Printer : __myProcess_Init : Error creating objects" );

		goto bailout;
	}

	unit->unit_PRT_Begin_MsgPortBit = 1UL << unit->unit_PRT_Begin_MsgPort->mp_SigBit;
	unit->unit_PRT_Abort_MsgPortBit = 1UL << unit->unit_PRT_Abort_MsgPort->mp_SigBit;

	// --

	if ( ( ! Printer_CloseTimerInit( unit ) ) || ( ! Process_Setup_Device( unit, msg ) ) )
	{
		USBERROR( "Printer : __myProcess_Init : Error setting up usb device" );

		goto bailout;
	}

	// --

	retval = TRUE;

bailout:

	return ( retval );
}

// --
