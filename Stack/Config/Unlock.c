
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

SEC_CODE void __Config_Unlock( struct USBBase *usbbase, struct USB2_ConfigNode *cn, STR file UNUSED )

#else

SEC_CODE void __Config_Unlock( struct USBBase *usbbase, struct USB2_ConfigNode *cn )

#endif

{
	TASK_NAME_ENTER( "__Config_Unlock" );

//	USBDEBUG( "__Config_Unlock          : CN %p : Locks %ld (old) : (%s)", cn, (cn)?cn->cfg_Locks:0, (file)?file:"<NULL>" );

	SEMAPHORE_OBTAIN( & usbbase->usb_LockSemaphore );

	if ( CONFIG_VALID(cn) == VSTAT_Okay )
	{
		if ( cn->cfg_Locks > 0 )
		{
			cn->cfg_Locks--;

//			USBDEBUG( "__Config_Unlock          : CN %p : New Lock Count : %ld (-1)", cn, cn->cfg_Locks );
		}
		else
		{
			USBPANIC( "__Config_Unlock          : CN %p : Config NOT Locked (%ld) : (%s)", cn, cn->cfg_Locks, (file)?file:"<NULL>" );
		}
	}
	else
	{
		USBPANIC( "__Config_Unlock          : CN %p : Invalid Config Node", cn );
	}

	SEMAPHORE_RELEASE( & usbbase->usb_LockSemaphore );

	TASK_NAME_LEAVE();
}

// --
