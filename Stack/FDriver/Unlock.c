
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

SEC_CODE void __FDriver_Unlock( struct USBBase *usbbase, struct USB2_FktDriverNode *fdn, STR file UNUSED )

#else

SEC_CODE void __FDriver_Unlock( struct USBBase *usbbase, struct USB2_FktDriverNode *fdn )

#endif

{
	TASK_NAME_ENTER( "__FDriver_Unlock" );

	USBDEBUG( "__FDriver_Unlock         : FDN %p : Locks %ld (old) : (%s)", fdn, ( fdn )?fdn->fdn_Locks:0, (file)?file:"<NULL>" );

	SEMAPHORE_OBTAIN( & usbbase->usb_LockSemaphore );

	if ( FDRIVER_VALID( fdn ) == VSTAT_Okay )
	{
		if ( fdn->fdn_Locks > 0 )
		{
			fdn->fdn_Locks--;

//			USBDEBUG( "__FDriver_Unlock         : FDN %p : New Lock Count : %ld (-1)", fdn, fdn->fdn_Locks );
		}
		else
		{
			USBPANIC( "__FDriver_Unlock         : FDN %p : FDriver Node NOT Locked (%ld)", fdn, fdn->fdn_Locks );
		}
	}
	else
	{
		USBERROR( "__FDriver_Unlock         : FDN %p : Invalid FDriver Node", fdn );
	}

	SEMAPHORE_RELEASE( & usbbase->usb_LockSemaphore );

	TASK_NAME_LEAVE();
}

// --
