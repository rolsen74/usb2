
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

#if defined( DO_PANIC ) || defined( DO_ERROR ) || defined( DO_DEBUG ) || defined( DO_INFO )

SEC_CODE U32 __Misc_DoCommand( struct USBBase *usbbase, struct CmdHeader *ch, STR file UNUSED )

#else

SEC_CODE U32 __Misc_DoCommand( struct USBBase *usbbase, struct CmdHeader *ch )

#endif

{
	struct USB2_Signal sig;

	U32 retval;

	TASK_NAME_ENTER( "__Misc_DoCommand" );

	retval = TASK_Return_Stack_Error;
	sig.sig_StructID = 0;

	if ( ! ch )
	{
		goto bailout;
	}

	if ( ! TASK_ALLOCSIGNAL( &sig ) )
	{
		goto bailout;
	}

	ch->cm_usbbase = usbbase;
	ch->cm_Result = TASK_Return_Stack_Error;
	ch->cm_Task = TASK_FIND();
	ch->cm_Mask = sig.sig_Signal_Mask;

	if ( USB2_Master_QueueCommand( usbbase, ch ) )
	{
		TASK_WAIT( sig.sig_Signal_Mask );

		retval = ch->cm_Result;
	}

	TASK_FREESIGNAL( &sig );

bailout:

	TASK_NAME_LEAVE();

	return ( retval );
}

// --
