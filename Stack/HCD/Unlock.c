
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

#if defined( DO_PANIC ) || defined( DO_ERROR ) || defined( DO_DEBUG ) || defined( DO_INFO )

SEC_CODE void __HCD_Unlock( struct USBBase *usbbase, struct USB2_HCDNode *hn, STR file UNUSED )

#else

SEC_CODE void __HCD_Unlock( struct USBBase *usbbase, struct USB2_HCDNode *hn )

#endif

{
	TASK_NAME_ENTER( "__HCD_Unlock" );

	USBDEBUG( "__HCD_Unlock             : HN %p : Locks %ld (old) : (%s)", hn, ( hn )?hn->hn_Locks:0, (file)?file:"<NULL>" );

	SEMAPHORE_OBTAIN( & usbbase->usb_LockSemaphore );

	if ( HCD_VALID( hn ) == VSTAT_Okay )
	{
		if ( hn->hn_Locks > 0 )
		{
			hn->hn_Locks--;

//			USBDEBUG( "__HCD_Unlock             : HN %p : New Lock Count : %ld (-1)", hn, hn->hn_Locks );
		}
		else
		{
			USBPANIC( "__HCD_Unlock             : HN %p : HN NOT Locked (%ld)", hn, hn->hn_Locks );
		}
	}
	else
	{
		USBPANIC( "__HCD_Unlock             : HN %p : Invalid HCD Node", hn );
	}

	SEMAPHORE_RELEASE( & usbbase->usb_LockSemaphore );

	TASK_NAME_LEAVE();
}

// --

