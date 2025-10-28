
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#include "All.h"

// --

static S32 __myProcess_Init( struct PrinterUnit *unit )
{
S32 retval;

	retval = FALSE;

	USBERROR( "Printer : __myProcess_Init" );

	// --

	if ( ! __myProcess_1_Get_Config( unit ))
	{
		USBERROR( "Printer : __myProcess_Init : Error parsing config" );
		goto bailout;
	}

	// --

	if ( ! Process_Find_Device( unit ))
	{
		USBERROR( "Printer : __myProcess_Init : Error did not find usb device" );
		goto bailout;
	}

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

	if ( ! Process_Setup_Device( unit ))
	{
		USBERROR( "Printer : __myProcess_Init : Error setting up usb device" );
		goto bailout;
	}

	// --

	retval = TRUE;

bailout:

	return( retval );
}

// --
