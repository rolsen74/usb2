
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

SEC_CODE void __IORequest_Active_Add( struct USBBase *usbbase, struct RealRequest *ioreq, STR file UNUSED )

#else

SEC_CODE void __IORequest_Active_Add( struct USBBase *usbbase, struct RealRequest *ioreq )

#endif

{
struct RealFunctionNode *fn;

	TASK_NAME_ENTER( "__IORequest_Active_Add" );

	if ( IOREQUEST_LOCK( ioreq ) == LSTAT_Okay )
	{
		fn = ioreq->req_Function;

		TASK_NAME_SET( "__IORequest_Active_Add : 2" );

		if ( FUNCTION_LOCK( fn ) == LSTAT_Okay )
		{
			TASK_NAME_SET( "__IORequest_Active_Add : 3" );

			SEMAPHORE_OBTAIN( & usbbase->usb_LockSemaphore );

			fn->fkt_ActiveIOReq++;

			SEMAPHORE_RELEASE( & usbbase->usb_LockSemaphore );

			FUNCTION_UNLOCK( fn );
		}
		else
		{
			USBPANIC( "__IORequest_Active_Add : Failed to lock : FN    %p : (%s)", fn, (file)?file:"<NULL>" );
		}

		TASK_NAME_SET( "__IORequest_Active_Add : 7" );

		IOREQUEST_UNLOCK( ioreq );
	}
	else
	{
		USBPANIC( "__IORequest_Active_Add : Failed to lock : IOReq %p : (%s)", ioreq, (file)?file:"<NULL>" );
	}

	TASK_NAME_LEAVE();
}

// --
