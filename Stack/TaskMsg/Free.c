
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#include "usb2_all.h"

// --
// The caller owns a message that has been removed from its queue or list.

#if defined( DO_PANIC ) || defined( DO_ERROR ) || defined( DO_DEBUG ) || defined( DO_INFO )

SEC_CODE enum FSTAT
__TaskMsg_Free( struct USBBase *usbbase, struct USB2_TaskMsg *tm, STR file UNUSED )

#else

SEC_CODE enum FSTAT
__TaskMsg_Free( struct USBBase *usbbase, struct USB2_TaskMsg *tm )

#endif

{
	enum VSTAT vstat;
	enum FSTAT fstat;

	TASK_NAME_ENTER( "__TaskMsg_Free" );

	vstat = TASKMSG_VALID( tm );

	if ( vstat != VSTAT_Okay )
	{
		fstat = ( vstat == VSTAT_Null ) ? FSTAT_Null : FSTAT_Error;
		goto bailout;
	}

	if ( tm->tm_TimeReq_Active )
	{
		USBERROR( "__TaskMsg_Free: timer still active for %p", tm );

		fstat = FSTAT_Locked;
		goto bailout;
	}

	tm->tm_StructID = ID_USB2_FREED;

	if ( tm->tm_Task )
	{
		TASK_UNLOCK( tm->tm_Task );
	}

	if ( tm->tm_HCD )
	{
		HCD_UNLOCK( tm->tm_HCD );
	}

	MEMORY_FREE( MEMID_USBTaskMsg, tm, 0 );

	fstat = FSTAT_Okay;

bailout:

	TASK_NAME_LEAVE();

	return ( fstat );
}

// --
