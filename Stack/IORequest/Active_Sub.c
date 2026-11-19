
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

SEC_CODE void __IORequest_Active_Sub( struct USBBase *usbbase, struct RealRequest *ioreq, STR file UNUSED )

#else

SEC_CODE void __IORequest_Active_Sub( struct USBBase *usbbase, struct RealRequest *ioreq )

#endif

{
struct RealFunctionNode *fn;

	TASK_NAME_ENTER( "__IORequest_Active_Sub" );

	if ( IOREQUEST_LOCK( ioreq ) == LSTAT_Okay )
	{
		fn = ioreq->req_Function;

		TASK_NAME_SET( "__IORequest_Active_Sub : 1" );

		if ( FUNCTION_LOCK( fn ) == LSTAT_Okay )
		{
			TASK_NAME_SET( "__IORequest_Active_Sub : 2" );

			SEMAPHORE_OBTAIN( & usbbase->usb_LockSemaphore );

			if ( fn->fkt_ActiveIOReq > 0 )
			{
				fn->fkt_ActiveIOReq--;
			}
			else
			{
				USBPANIC( "__IORequest_Active_Sub : IOReq %p : IORequest NOT Active (%ld) : (%s)", ioreq, fn->fkt_ActiveIOReq, (file)?file:"<NULL>" );
			}

			SEMAPHORE_RELEASE( & usbbase->usb_LockSemaphore );

			FUNCTION_UNLOCK( fn );
		}
		else
		{
			USBPANIC( "__IORequest_Active_Sub : Failed to lock : FN    %p : (%s)", fn, (file)?file:"<NULL>" );
		}

		TASK_NAME_SET( "__IORequest_Active_Sub : 6" );

		IOREQUEST_UNLOCK( ioreq );
	}
	else
	{
		USBPANIC( "__IORequest_Active_Sub : Failed to lock : IOReq %p : (%s)", ioreq, (file)?file:"<NULL>" );
	}

	TASK_NAME_LEAVE();
}

// --
